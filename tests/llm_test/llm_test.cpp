
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

/*
 * ============================================================================
 *  llm_test.cpp  ——  从零手写一个最小 GPT-style Transformer (CPU / pure C++)
 * ----------------------------------------------------------------------------
 *  目标：用最少的代码量，最线性的控制流，把现代 LLM 的核心机制讲清楚：
 *      Embedding -> N x ( LayerNorm -> Self-Attention -> LayerNorm -> MLP )
 *                -> LayerNorm -> LM-Head -> Softmax -> CE Loss
 *
 *  实施分两轮：
 *      [Round 1 - 当前]  数学基础(Mat) + 模型前向 + sanity check
 *      [Round 2 - 下一轮] 反向传播 + 数值梯度对照 + SGD/AdamW + 训练 + 采样
 *
 *  设计原则：
 *      - 单文件、零外部依赖（仅 std + 项目自带 fn_log/zprof/test_common）
 *      - struct 仅作数据容器，所有逻辑都在自由函数里 -> 控制流线性
 *      - 数值类型统一 float；矩阵 row-major，平铺到 std::vector<float>
 * ============================================================================
 */

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <algorithm>
#include <cassert>

#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"


/* ============================================================================
 * Part 1.  基础数学库
 *   - struct Mat : row-major 矩阵
 *   - 自由函数风格的算子：matmul / transpose / softmax / gelu / layernorm ...
 *   - 末尾 test_math_basics() 作单元自测
 * ============================================================================
 */

struct Mat
{
    int rows;
    int cols;
    std::vector<float> data;   // size = rows * cols, row-major: a[i][j] = data[i*cols + j]
};

static inline Mat mat_zeros(int r, int c)
{
    Mat m;
    m.rows = r;
    m.cols = c;
    m.data.assign((size_t)r * (size_t)c, 0.0f);
    return m;
}

static inline float& mat_at(Mat& m, int i, int j)
{
    return m.data[(size_t)i * m.cols + j];
}
static inline float mat_at(const Mat& m, int i, int j)
{
    return m.data[(size_t)i * m.cols + j];
}

/* Xavier/Glorot 均匀分布初始化：U(-a, a), a = sqrt(6 / (fan_in + fan_out))
 * 这是 Linear 层权重的常用初始化方式，可让信号方差大致跨层守恒。 */
static void mat_xavier(Mat& m, std::mt19937& rng)
{
    float a = std::sqrt(6.0f / (float)(m.rows + m.cols));
    std::uniform_real_distribution<float> dist(-a, a);
    for (auto& v : m.data) v = dist(rng);
}

/* 小幅正态初始化：用于 embedding 等 */
static void mat_randn(Mat& m, float stddev, std::mt19937& rng)
{
    std::normal_distribution<float> dist(0.0f, stddev);
    for (auto& v : m.data) v = dist(rng);
}

/* C = A @ B   shape: (M,K) @ (K,N) -> (M,N)
 * 这是整个 Transformer 中出现频率最高、计算量最大的算子。
 * 朴素三重循环；玩具规模够用，不做任何优化。 */
static Mat matmul(const Mat& A, const Mat& B)
{
    assert(A.cols == B.rows);
    Mat C = mat_zeros(A.rows, B.cols);
    for (int i = 0; i < A.rows; ++i)
    {
        for (int k = 0; k < A.cols; ++k)
        {
            float a = mat_at(A, i, k);
            for (int j = 0; j < B.cols; ++j)
            {
                C.data[(size_t)i * C.cols + j] += a * mat_at(B, k, j);
            }
        }
    }
    return C;
}

/* B = A^T */
static Mat transpose(const Mat& A)
{
    Mat B = mat_zeros(A.cols, A.rows);
    for (int i = 0; i < A.rows; ++i)
        for (int j = 0; j < A.cols; ++j)
            mat_at(B, j, i) = mat_at(A, i, j);
    return B;
}

/* A += B */
static void mat_add_inplace(Mat& A, const Mat& B)
{
    assert(A.rows == B.rows && A.cols == B.cols);
    for (size_t i = 0; i < A.data.size(); ++i) A.data[i] += B.data[i];
}

/* A *= s */
static void mat_scale_inplace(Mat& A, float s)
{
    for (auto& v : A.data) v *= s;
}

/* 行 softmax（数值稳定：先减每行 max）
 * y[i][j] = exp(x[i][j] - max_i) / sum_j exp(x[i][j] - max_i)
 */
static void softmax_rows_inplace(Mat& X)
{
    for (int i = 0; i < X.rows; ++i)
    {
        float* row = &X.data[(size_t)i * X.cols];
        float mx = row[0];
        for (int j = 1; j < X.cols; ++j) if (row[j] > mx) mx = row[j];
        float sum = 0.0f;
        for (int j = 0; j < X.cols; ++j)
        {
            row[j] = std::exp(row[j] - mx);
            sum += row[j];
        }
        float inv = 1.0f / sum;
        for (int j = 0; j < X.cols; ++j) row[j] *= inv;
    }
}

/* GELU 激活（GPT/BERT 标准）
 *   GELU(x) ≈ 0.5 x (1 + tanh( sqrt(2/pi) (x + 0.044715 x^3) ))
 * 与 ReLU 的区别：
 *   ReLU: x<0 一律 0，导数 0/1 阶跃；
 *   GELU: x<0 时仍有少量负输出，处处可导且单调平滑。Transformer 实测优于 ReLU。
 */
static inline float gelu_scalar(float x)
{
    const float kSqrt2OverPi = 0.7978845608028654f;
    float t = kSqrt2OverPi * (x + 0.044715f * x * x * x);
    return 0.5f * x * (1.0f + std::tanh(t));
}
static inline float relu_scalar(float x) { return x > 0.0f ? x : 0.0f; }

static void gelu_inplace(Mat& X)
{
    for (auto& v : X.data) v = gelu_scalar(v);
}

/* LayerNorm（对每一行独立归一化）
 *   mu = mean(x_row);  var = mean((x-mu)^2)
 *   y = (x - mu) / sqrt(var + eps) * gamma + beta
 * 与 BatchNorm 的区别：BN 对一个 batch 内同一通道做统计，依赖 batch；
 *                        LN 对单个样本的 feature 维做统计，与 batch 无关 ——
 *                        非常适合变长序列模型，所以 Transformer 用 LN。
 */
struct LayerNormParam
{
    std::vector<float> gamma;   // size = dim
    std::vector<float> beta;    // size = dim
};

static void layernorm_init(LayerNormParam& ln, int dim)
{
    ln.gamma.assign(dim, 1.0f);
    ln.beta.assign(dim, 0.0f);
}

static Mat layernorm_forward(const Mat& X, const LayerNormParam& ln, float eps = 1e-5f)
{
    assert((int)ln.gamma.size() == X.cols);
    Mat Y = mat_zeros(X.rows, X.cols);
    for (int i = 0; i < X.rows; ++i)
    {
        const float* xr = &X.data[(size_t)i * X.cols];
        float* yr = &Y.data[(size_t)i * Y.cols];
        float mean = 0.0f;
        for (int j = 0; j < X.cols; ++j) mean += xr[j];
        mean /= (float)X.cols;
        float var = 0.0f;
        for (int j = 0; j < X.cols; ++j) { float d = xr[j] - mean; var += d * d; }
        var /= (float)X.cols;
        float inv = 1.0f / std::sqrt(var + eps);
        for (int j = 0; j < X.cols; ++j)
            yr[j] = (xr[j] - mean) * inv * ln.gamma[j] + ln.beta[j];
    }
    return Y;
}

/* ----- 数学库自测 ----- */
static int test_math_basics()
{
    LogInfo() << "[Part1] test_math_basics begin";

    /* matmul: [[1,2],[3,4]] @ [[5,6],[7,8]] = [[19,22],[43,50]] */
    Mat A = mat_zeros(2, 2); A.data = {1, 2, 3, 4};
    Mat B = mat_zeros(2, 2); B.data = {5, 6, 7, 8};
    Mat C = matmul(A, B);
    if (!(std::fabs(C.data[0] - 19.0f) < 1e-5f &&
          std::fabs(C.data[1] - 22.0f) < 1e-5f &&
          std::fabs(C.data[2] - 43.0f) < 1e-5f &&
          std::fabs(C.data[3] - 50.0f) < 1e-5f))
    {
        LogError() << "matmul FAILED: " << C.data[0] << "," << C.data[1] << "," << C.data[2] << "," << C.data[3];
        return -1;
    }

    /* transpose */
    Mat At = transpose(A);
    if (!(At.rows == 2 && At.cols == 2 &&
          std::fabs(At.data[0] - 1.0f) < 1e-5f &&
          std::fabs(At.data[1] - 3.0f) < 1e-5f &&
          std::fabs(At.data[2] - 2.0f) < 1e-5f &&
          std::fabs(At.data[3] - 4.0f) < 1e-5f))
    {
        LogError() << "transpose FAILED";
        return -1;
    }

    /* softmax: 每行和应为 1，单调性保持 */
    Mat S = mat_zeros(1, 3); S.data = {1.0f, 2.0f, 3.0f};
    softmax_rows_inplace(S);
    float sum = S.data[0] + S.data[1] + S.data[2];
    if (std::fabs(sum - 1.0f) > 1e-5f)
    {
        LogError() << "softmax sum FAILED: " << sum;
        return -1;
    }
    if (!(S.data[0] < S.data[1] && S.data[1] < S.data[2]))
    {
        LogError() << "softmax monotonicity FAILED";
        return -1;
    }

    /* GELU 关键点：GELU(0)=0, GELU 在 0 附近 < ReLU，但在大正数时趋同 */
    if (std::fabs(gelu_scalar(0.0f)) > 1e-6f)
    {
        LogError() << "GELU(0) != 0";
        return -1;
    }
    if (!(gelu_scalar(3.0f) > 2.9f))   // x=3 时 GELU≈x
    {
        LogError() << "GELU(3) too small: " << gelu_scalar(3.0f);
        return -1;
    }
    /* 对比打印：让你直观看到 GELU vs ReLU 差异 */
    LogInfo() << "  GELU(-2)=" << gelu_scalar(-2.0f) << "  ReLU(-2)=" << relu_scalar(-2.0f);
    LogInfo() << "  GELU(-0.5)=" << gelu_scalar(-0.5f) << "  ReLU(-0.5)=" << relu_scalar(-0.5f);
    LogInfo() << "  GELU(0.5)=" << gelu_scalar(0.5f) << "  ReLU(0.5)=" << relu_scalar(0.5f);
    LogInfo() << "  GELU(2)=" << gelu_scalar(2.0f) << "  ReLU(2)=" << relu_scalar(2.0f);

    /* LayerNorm: 输出每行均值≈0, 方差≈1（gamma=1,beta=0时） */
    LayerNormParam ln;
    layernorm_init(ln, 4);
    Mat X = mat_zeros(1, 4); X.data = {1.0f, 2.0f, 3.0f, 4.0f};
    Mat Y = layernorm_forward(X, ln);
    float ymean = 0.0f;
    for (int j = 0; j < 4; ++j) ymean += Y.data[j];
    ymean /= 4.0f;
    if (std::fabs(ymean) > 1e-4f)
    {
        LogError() << "layernorm mean != 0: " << ymean;
        return -1;
    }

    LogInfo() << "[Part1] test_math_basics PASS";
    return 0;
}


/* ============================================================================
 * Part 2.  Transformer 模型（前向）
 *
 *  超参（玩具规模，确保 CPU 秒级训练）：
 *      VOCAB    : 词表大小（运行时根据训练文本动态决定）
 *      D_MODEL  : token 嵌入维度
 *      N_HEAD   : 注意力头数（D_MODEL 必须能被 N_HEAD 整除）
 *      D_HEAD   : 单头维度 = D_MODEL / N_HEAD
 *      D_FF     : MLP 隐层维度（GPT 惯例 = 4 * D_MODEL，这里也用 2x 即可）
 *      N_LAYER  : Transformer block 层数
 *      CTX_LEN  : 最大上下文长度（位置嵌入表大小）
 *
 *  关键点：causal mask —— 自回归 LM 的灵魂
 *      在 attention scores 矩阵 (T,T) 中，把上三角（j>i）置为 -inf，
 *      softmax 后这些位置变 0，于是 token i 只能"看见"位置 ≤ i 的 token。
 *      没有这一步，模型在训练时就"作弊"看到未来 token，预测任务退化。
 * ============================================================================
 */

static const int   D_MODEL = 16;
static const int   N_HEAD  = 2;
static const int   D_HEAD  = D_MODEL / N_HEAD;     // = 8
static const int   D_FF    = 32;
static const int   N_LAYER = 1;
static const int   CTX_LEN = 8;

/* 单层注意力块。所有 W* 都是 (D_MODEL, D_MODEL)。
 * 缓存的中间张量在 Round 2 反向传播时会用到，这一轮先只填充。 */
struct AttentionBlock
{
    Mat Wq, Wk, Wv, Wo;
};

struct MLPBlock
{
    Mat W1;   // (D_MODEL, D_FF)
    Mat W2;   // (D_FF, D_MODEL)
};

struct TransformerBlock
{
    LayerNormParam ln1;
    AttentionBlock attn;
    LayerNormParam ln2;
    MLPBlock       mlp;
};

struct Model
{
    int vocab;
    Mat token_emb;        // (VOCAB, D_MODEL)
    Mat pos_emb;          // (CTX_LEN, D_MODEL)
    std::vector<TransformerBlock> blocks;   // size = N_LAYER
    LayerNormParam ln_final;
    Mat lm_head;          // (D_MODEL, VOCAB)
};

static void model_init(Model& m, int vocab, std::mt19937& rng)
{
    m.vocab = vocab;
    m.token_emb = mat_zeros(vocab, D_MODEL);    mat_randn(m.token_emb, 0.02f, rng);
    m.pos_emb   = mat_zeros(CTX_LEN, D_MODEL);  mat_randn(m.pos_emb, 0.02f, rng);

    m.blocks.resize(N_LAYER);
    for (auto& blk : m.blocks)
    {
        layernorm_init(blk.ln1, D_MODEL);
        layernorm_init(blk.ln2, D_MODEL);
        blk.attn.Wq = mat_zeros(D_MODEL, D_MODEL); mat_xavier(blk.attn.Wq, rng);
        blk.attn.Wk = mat_zeros(D_MODEL, D_MODEL); mat_xavier(blk.attn.Wk, rng);
        blk.attn.Wv = mat_zeros(D_MODEL, D_MODEL); mat_xavier(blk.attn.Wv, rng);
        blk.attn.Wo = mat_zeros(D_MODEL, D_MODEL); mat_xavier(blk.attn.Wo, rng);
        blk.mlp.W1  = mat_zeros(D_MODEL, D_FF);    mat_xavier(blk.mlp.W1, rng);
        blk.mlp.W2  = mat_zeros(D_FF, D_MODEL);    mat_xavier(blk.mlp.W2, rng);
    }
    layernorm_init(m.ln_final, D_MODEL);
    m.lm_head = mat_zeros(D_MODEL, vocab); mat_xavier(m.lm_head, rng);
}


/* ----- 注意力前向 -----
 * 输入 X: (T, D_MODEL)  T = 当前序列长度 (<= CTX_LEN)
 * 输出   : (T, D_MODEL)
 *
 * 多头实现思路：先按 D_MODEL 整体投影出 Q/K/V，然后按 head 切分通道：
 *   Q = X @ Wq        (T, D_MODEL)
 *   把 Q 视作 N_HEAD 个 (T, D_HEAD) 子矩阵；K/V 同理。
 *   每个 head 独立做 scaled-dot-product attention；
 *   再把 N_HEAD 个 (T, D_HEAD) 拼回 (T, D_MODEL)，过 Wo 输出投影。
 */
static Mat attention_forward(const Mat& X, const AttentionBlock& a)
{
    const int T = X.rows;
    Mat Q = matmul(X, a.Wq);   // (T, D_MODEL)
    Mat K = matmul(X, a.Wk);
    Mat V = matmul(X, a.Wv);

    Mat Out = mat_zeros(T, D_MODEL);
    const float scale = 1.0f / std::sqrt((float)D_HEAD);

    /* 逐 head 处理 */
    for (int h = 0; h < N_HEAD; ++h)
    {
        const int off = h * D_HEAD;

        /* 抽取本 head 的 Qh/Kh/Vh : (T, D_HEAD) */
        Mat Qh = mat_zeros(T, D_HEAD);
        Mat Kh = mat_zeros(T, D_HEAD);
        Mat Vh = mat_zeros(T, D_HEAD);
        for (int i = 0; i < T; ++i)
            for (int j = 0; j < D_HEAD; ++j)
            {
                mat_at(Qh, i, j) = mat_at(Q, i, off + j);
                mat_at(Kh, i, j) = mat_at(K, i, off + j);
                mat_at(Vh, i, j) = mat_at(V, i, off + j);
            }

        /* scores = Qh @ Kh^T  : (T, T)，再 / sqrt(d_head) */
        Mat KhT = transpose(Kh);
        Mat scores = matmul(Qh, KhT);
        mat_scale_inplace(scores, scale);

        /* causal mask: 上三角 (j > i) 置 -inf */
        const float kNegInf = -1e30f;
        for (int i = 0; i < T; ++i)
            for (int j = i + 1; j < T; ++j)
                mat_at(scores, i, j) = kNegInf;

        /* softmax 行归一化 -> 注意力权重 */
        softmax_rows_inplace(scores);

        /* head_out = scores @ Vh : (T, D_HEAD)，写回 Out 的对应通道 */
        Mat Hh = matmul(scores, Vh);
        for (int i = 0; i < T; ++i)
            for (int j = 0; j < D_HEAD; ++j)
                mat_at(Out, i, off + j) = mat_at(Hh, i, j);
    }

    /* 输出投影 Wo */
    return matmul(Out, a.Wo);
}

/* ----- MLP 前向 -----
 * y = GELU(x @ W1) @ W2     (T, D_MODEL) -> (T, D_FF) -> (T, D_MODEL)
 * 注：GPT 的 MLP 通常还有 bias，这里玩具版省略 bias 让代码更紧凑。
 */
static Mat mlp_forward(const Mat& X, const MLPBlock& m)
{
    Mat H = matmul(X, m.W1);
    gelu_inplace(H);
    return matmul(H, m.W2);
}

/* ----- 单层 Transformer block 前向 -----
 * 采用 Pre-LN 结构（更易训练，GPT-2/3 同款）：
 *   x = x + Attention( LN1(x) )
 *   x = x + MLP      ( LN2(x) )
 */
static Mat block_forward(const Mat& X, const TransformerBlock& blk)
{
    Mat h = layernorm_forward(X, blk.ln1);
    Mat a = attention_forward(h, blk.attn);
    Mat x1 = X;
    mat_add_inplace(x1, a);                 // 残差

    Mat h2 = layernorm_forward(x1, blk.ln2);
    Mat m = mlp_forward(h2, blk.mlp);
    Mat x2 = x1;
    mat_add_inplace(x2, m);                 // 残差
    return x2;
}

/* ----- 整模型前向 -----
 *  tokens: 长度 T 的整型序列  (T <= CTX_LEN)
 *  返回 logits: (T, VOCAB)
 *      logits[i, :] 表示"已看见 token[0..i] 时，对下一个 token 的未归一化打分"
 */
static Mat model_forward(const Model& m, const std::vector<int>& tokens)
{
    const int T = (int)tokens.size();
    assert(T > 0 && T <= CTX_LEN);

    /* 输入嵌入 = token 嵌入 + 位置嵌入 */
    Mat X = mat_zeros(T, D_MODEL);
    for (int i = 0; i < T; ++i)
    {
        int tok = tokens[i];
        assert(tok >= 0 && tok < m.vocab);
        for (int j = 0; j < D_MODEL; ++j)
            mat_at(X, i, j) = mat_at(m.token_emb, tok, j) + mat_at(m.pos_emb, i, j);
    }

    /* 堆叠 N_LAYER 个 block */
    for (const auto& blk : m.blocks)
        X = block_forward(X, blk);

    /* 最终 LayerNorm + LM Head 投影到词表空间 */
    X = layernorm_forward(X, m.ln_final);
    return matmul(X, m.lm_head);   // (T, VOCAB)
}


/* ============================================================================
 * Part 6 (前置). main —— sanity check
 *  验证：
 *    1) 数学库正确
 *    2) 前向 shape 正确，输出无 NaN/Inf
 *    3) softmax 后每行和 ≈ 1
 *    4) causal mask 生效：改变位置 i+1 的 token 不应改变位置 i 的 logits
 * ============================================================================
 */

static int sanity_check_forward()
{
    LogInfo() << "[Part2] sanity_check_forward begin";

    std::mt19937 rng(20260515);
    const int VOCAB = 5;
    Model m;
    model_init(m, VOCAB, rng);
    LogInfo() << "  model inited: vocab=" << VOCAB
              << " D_MODEL=" << D_MODEL << " N_HEAD=" << N_HEAD
              << " D_FF=" << D_FF << " N_LAYER=" << N_LAYER
              << " CTX_LEN=" << CTX_LEN;

    /* 1) 前向跑通，输出形状 (T, VOCAB) */
    std::vector<int> seq1 = {0, 1, 2, 3};
    Mat logits = model_forward(m, seq1);
    if (logits.rows != (int)seq1.size() || logits.cols != VOCAB)
    {
        LogError() << "logits shape wrong: " << logits.rows << "x" << logits.cols;
        return -1;
    }

    /* 2) NaN/Inf 检查 */
    for (float v : logits.data)
    {
        if (!std::isfinite(v))
        {
            LogError() << "logits contains NaN/Inf";
            return -1;
        }
    }

    /* 3) softmax 行和 ≈ 1 */
    Mat probs = logits;
    softmax_rows_inplace(probs);
    for (int i = 0; i < probs.rows; ++i)
    {
        float s = 0.0f;
        for (int j = 0; j < probs.cols; ++j) s += mat_at(probs, i, j);
        if (std::fabs(s - 1.0f) > 1e-4f)
        {
            LogError() << "softmax row " << i << " sum=" << s;
            return -1;
        }
    }
    LogInfo() << "  forward shape & finiteness & softmax OK";

    /* 4) causal mask 生效检验
     *    构造两个序列，仅最后一个 token 不同；前 T-1 个位置的 logits 应完全相同。
     *    这是自回归 LM 训练正确性的"关键铁证"。
     */
    std::vector<int> seqA = {0, 1, 2, 3};
    std::vector<int> seqB = {0, 1, 2, 4};   // 只改最后一位
    Mat lA = model_forward(m, seqA);
    Mat lB = model_forward(m, seqB);
    float max_diff_prefix = 0.0f;   // 位置 0..T-2
    float max_diff_last   = 0.0f;   // 位置 T-1
    for (int i = 0; i < lA.rows; ++i)
    {
        for (int j = 0; j < lA.cols; ++j)
        {
            float d = std::fabs(mat_at(lA, i, j) - mat_at(lB, i, j));
            if (i < lA.rows - 1) max_diff_prefix = (std::max)(max_diff_prefix, d);
            else                 max_diff_last   = (std::max)(max_diff_last, d);
        }
    }
    LogInfo() << "  causal mask check: max_diff_prefix=" << max_diff_prefix
              << " (should ~0)   max_diff_last=" << max_diff_last
              << " (should >0)";
    if (max_diff_prefix > 1e-5f)
    {
        LogError() << "causal mask FAILED: front positions changed!";
        return -1;
    }
    if (max_diff_last < 1e-6f)
    {
        LogError() << "last position did not change?? something is off";
        return -1;
    }

    /* 5) 打印第 0 行的概率分布，给一个直观感受 */
    LogInfo() << "  prob[pos=0]: "
              << mat_at(probs, 0, 0) << " "
              << mat_at(probs, 0, 1) << " "
              << mat_at(probs, 0, 2) << " "
              << mat_at(probs, 0, 3) << " "
              << mat_at(probs, 0, 4);

    LogInfo() << "[Part2] sanity_check_forward PASS";
    return 0;
}


int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    ztest_init();

    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin and exit");

    LogDebug() << " main begin test. ";

    float c = 0;
    c = gelu_scalar(-3.0f);
    c = gelu_scalar(-2.0f);
    c = gelu_scalar(-1.0f);
    c = gelu_scalar(0.0f);
    c = gelu_scalar(1.0f);
    c = gelu_scalar(2.0f);
    c = gelu_scalar(3.0f);
    c = gelu_scalar(4.0f);
    c = gelu_scalar(5.0f);


    /* 线性串起来：每一步都必须 PASS，否则提前返回 */
    if (test_math_basics() != 0)         return -1;
    if (sanity_check_forward() != 0)     return -2;

    LogInfo() << "[Round 1] forward-only build PASS. "
              << "Next round will add backward + SGD/AdamW + training + sampling.";

    LogInfo() << "all test finish .";
    return 0;
}


