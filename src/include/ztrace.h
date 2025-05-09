

/*
* Copyright (C) 2015 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

//old repo : https://github.com/zsummer/traceback  


#pragma once 
#ifndef ZTRACE_H
#define ZTRACE_H

#ifdef WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <iostream>
#include <windows.h>
#include <DbgHelp.h>
#include <string>
#pragma comment(lib, "Dbghelp")
#else
#include <execinfo.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdint.h>
#include <type_traits>
#include <iterator>
#include <cstddef>
#include <memory>
#include <algorithm>
#include <string>


//default use format compatible short type .  
#if !defined(ZBASE_USE_AHEAD_TYPE) && !defined(ZBASE_USE_DEFAULT_TYPE)
#define ZBASE_USE_DEFAULT_TYPE
#endif 

//win & unix format incompatible   
#ifdef ZBASE_USE_AHEAD_TYPE
using s8 = int8_t;
using u8 = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using s64 = int64_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;
#endif

#ifdef ZBASE_USE_DEFAULT_TYPE
using s8 = char;
using u8 = unsigned char;
using s16 = short int;
using u16 = unsigned short int;
using s32 = int;
using u32 = unsigned int;
using s64 = long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;
#endif


#if __GNUG__
#define ZBASE_ALIAS __attribute__((__may_alias__))
#else
#define ZBASE_ALIAS
#endif

/* type_traits:
*
* is_trivially_copyable: in part
    * memset: uninit or no dync heap
    * memcpy: uninit or no dync heap
* shm resume : safely, require heap address fixed
    * has vptr:     no
    * static var:   no
    * has heap ptr: yes
    * has code ptr: no
* thread safe: read safe
*
*/




//针对调用链追踪 避免同步调用中出现过深的调用或者递归循环, 例如常见的事件系统, 技能系统, BUFF系统等. 这种情况一般栈帧数据使用业务ID.  
// * 当出现调用链错误码时, 通过栈帧中是否出现相同ID判断为出现递归循环, 否则通常为调用链过深(过深与否取决于业务)        
// * 当出现调用链错误码时, 通常应当立即退出函数来终止调用(小心判断和处理资源泄露问题).   
// * 调用中不应当轮询栈数据, 只通过是否错误来判断, 避免过多记录消耗  
// 
//针对调用链追踪 在出现错误时候打印id或者地址来重现调用关系   
// * 当出现调用链错误码时, 可以查看栈底数据来判断最后一次退栈记录(未覆盖的栈帧历史记录)      
// * 当出现调用链错误码时, 可以查看当前堆栈来源 
// * 不要求每层调用都guard, 取决于是否需要关注    
// * 调用栈可以用__FUNCTION__的地址来, 可以直接观察到函数调用关系 但因为PIC/PIE不能存储到shm  
// * 也可以存储业务来源ID, 快速排查调用的关键来源  


template<class _Ty =u32, s32 _Depth = 80>
class ztrace
{
public:
    enum TRACE_ERRCODE
    {
        TE_NONE,
        TE_STACKOVER, //需要手动清理  
        TE_MISMATCHING, //使用方式错误 
    };

    using value_type = _Ty;
    static constexpr s32 max_depth = _Depth;

public:
    ztrace()
    {
        static_assert(std::is_trivial<_Ty>::value, "");
        reset(false);
    }
    void reset(bool light_clear = true)
    {
        if (!light_clear)
        {
            memset(stacker_, 0, sizeof(stacker_));
        }
        size_ = 0;
        errcode_ = TE_NONE;
    }
    bool good() const 
    {
        return errcode_ == TE_NONE;
    }
    void set_errcode(s32 errcode = TE_NONE) { errcode_ = errcode; }
    s32 errcode()const
    {
        return errcode_;
    }
    void push(const _Ty& v)
    {
        if (errcode_ != TE_NONE)
        {
            return;
        }
        if (size_ >= _Depth)
        {
            errcode_ = TE_STACKOVER;
            return;
        }
        stacker_[size_++] = v;
    }
    void pop(const _Ty& v)
    {
        if (errcode_ != TE_NONE)
        {
            return;
        }
        if (size_ <= 0)
        {
            errcode_ = TE_MISMATCHING;
            return;
        }

        const _Ty& vtop = stacker_[size_ - 1];
        if (vtop != v)
        {
            errcode_ = TE_MISMATCHING;
            return;
        }
        size_--;
    }
    void set_top(s32 id) { size_ = id + 1; }
    //指向栈顶数据   
    s32 top()const { return size_ - 1; }
    s32 max_top()const { return max_depth -1; }
    const _Ty& at(s32 id)const { return stacker_[id]; }
    
    //utils
    static std::string traceback(bool short_trace = true);
public:
private:
    _Ty stacker_[_Depth];
    s32 size_;
    s32 errcode_;
};



template<class _Stacker>
class ztrace_guard
{
public:
    ztrace_guard() = delete;
    ztrace_guard<_Stacker>& operator=(ztrace_guard<_Stacker>&) = delete;
    explicit ztrace_guard(_Stacker& stacker, const typename _Stacker::value_type& v) :stacker_(stacker), v_(v)
    {
        stacker_.push(v_);
    }

    ~ztrace_guard()
    {
        stacker_.pop(v_);
    }
private:
    _Stacker& stacker_;
    const typename _Stacker::value_type v_;
};



//don't store to shm 
template<s32 _Depth>
using zcallstacker = ztrace<const char*, _Depth>;







template<class _Ty, s32 _Depth>
inline std::string ztrace<_Ty, _Depth>::traceback(bool short_trace)
{
    std::stringstream ss;
#ifdef WIN32

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE))
    {
        ss << "SymInitialize returned error " << GetLastError();
        return ss.str();
    }

    //     typedef USHORT(WINAPI *CaptureStackBackTraceType)(__in ULONG, __in ULONG, __out PVOID*, __out_opt PULONG);
    //     CaptureStackBackTraceType capture = (CaptureStackBackTraceType)(GetProcAddress(LoadLibraryA("kernel32.dll"), "RtlCaptureStackBackTrace"));
    //     if (capture == NULL) return;
    const int stackMax = 128;
    void* trace[stackMax];
    //    int count = (capture)(0, stackMax, trace, NULL);
    int count = (CaptureStackBackTrace)(0, stackMax, trace, NULL);
    for (int i = 1; i < count; i++)
    {
        ULONG64 buffer[(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 dwDisplacement = 0;
        if (SymFromAddr(GetCurrentProcess(), (DWORD64)trace[i], &dwDisplacement, pSymbol))
        {
            ss << "bt[" << i - 1 << "]       --[ " << pSymbol->Name << " ]--              from     ";
        }
        else
        {
            ss << "bt[" << i - 1 << "]   " << "error[" << GetLastError() << "]              from     ";
        }

        IMAGEHLP_LINE64 lineInfo = { sizeof(IMAGEHLP_LINE64) };
        DWORD dwLineDisplacement;
        if (SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)trace[i], &dwLineDisplacement, &lineInfo))
        {
            std::string pathfile = lineInfo.FileName;
            if (pathfile.empty())
            {
                ss << "\r\n";
                continue;
            }
            std::for_each(pathfile.begin(), pathfile.end(), [](char& ch) { if (ch == '/') ch = '\\'; });
            auto pos = pathfile.find_last_of('\\');
            if (pos != std::string::npos) pathfile[pos] = '/';
            pos = pathfile.find_last_of('\\');
            if (pos != std::string::npos) pathfile[pos] = '/'; else pos = -1;
            ss << pathfile.substr(pos + 1) << ":" << lineInfo.LineNumber;
        }
        else
        {
            ss << "------:0";
        }
        ss << "\r\n";
        if (strcmp(pSymbol->Name, "main") == 0) break;
    }
#else
    void* stack[200];
    size_t size = backtrace(stack, 200);
    
    ss << "backtrace: ";
    for (size_t i = 1; i < size; i++)
    {
        ss << stack[i] << "  ";
    }
    if (!short_trace)
    {
        ss << "\r\n";
        char** stackSymbol = backtrace_symbols(stack, size);
        for (size_t i = 1; i < size && stackSymbol != NULL; i++)
        {
            ss << "bt[" << i - 1 << "] " << stackSymbol[i] << "\r\n";
        }
        free(stackSymbol);
    }
    

    /*
    gdb info line * 0x40000000
    addr2line -f -C -e ./test  0x400fce  0x401027  0x7f2bfb401b45  0x400ee9
    */

#endif
    return ss.str();
}



#endif