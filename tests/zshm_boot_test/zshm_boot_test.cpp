/*
* zshm_loader License
* Copyright (C) 2014-2021 YaweiZhang <yawei.zhang@foxmail.com>.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#include "fn_log.h"
#include "zprof.h"
#include "test_common.h"
#include "zshm_loader.h"
#include <memory>
#include <zarray.h>
#include "zshm_boot.h"
#include "zbuddy.h"
#include "zmalloc.h"
#include "zshm_ptr.h"

using shm_header = zarray<u32, 100>;

static constexpr u32 kPageOrder = 20; //1m  
static constexpr u32 kDynSpaceOrder = 8; // 8:256,  10:1024  

#define SPACE_ALIGN(bytes) zmalloc_align_value(bytes, 16)

enum  ShmSpace : u32
{
    kFrame = 0,
    kObjectPool,
    kBuddy,
    kMalloc,
    kDyn,
};




class BaseFrame
{
public:
    virtual s32 Start()
    {
        return 0;
    }
    virtual s32 Resume()
    {
        return 0;
    }
    virtual s32 Init()
    {
        return 0;
    }
private:

};


template <class Frame>  //derive from BaseFrame  
class FrameDelegate
{
public:
    static_assert(std::is_base_of<BaseFrame, Frame>::value, "");
public:
    static Frame& Instance()
    {
        return *space<Frame, ShmSpace::kFrame>();
    }

    static inline char*& ShmInstance()
    {
        static char* g_instance_ptr = nullptr;
        return g_instance_ptr;
    }

    static inline zshm_space_entry& SpaceEntry()
    {
        return *(zshm_space_entry*)(ShmInstance());
    }
    template <class T, u32 ID>
    static inline T* space()
    {
        return (T*)(ShmInstance() + SpaceEntry().spaces_[ID].offset_);
    }
private:
    template<class T, class ... Args>
    static inline void RebuildVPTR(void* addr, Args ... args)
    {
        zshm_ptr<T>(addr).fix_vptr();
    }
    template<class T, class ... Args>
    static inline void BuildObject(void* addr, Args ... args)
    {
        new (addr) T(args...);
    }
    template<class T>
    static inline void DestroyObject(T* addr)
    {
        addr->~T();
    }

    static inline void* AllocLarge(u64 bytes)
    {
        bytes += zbuddy_shift_size(kPageOrder) - 1;
        u32 pages = (u32)(bytes >> kPageOrder);
        u64 page_index = space<zbuddy, ShmSpace::kBuddy>()->alloc_page(pages);
        void* addr = space<char, ShmSpace::kDyn>() + (page_index << kPageOrder);
        return addr;
    }
    static inline u64 FreeLarge(void*addr, u64 bytes)
    {
        u64 offset = (char*)addr - space<char, ShmSpace::kDyn>();
        u32 page_index = (u32)(offset >> kPageOrder);
        u32 pages = space<zbuddy, ShmSpace::kBuddy>()->free_page(page_index);
        u64 free_bytes = pages << kPageOrder;
        if (free_bytes < bytes)
        {
            LogError() << "";
        }
        return free_bytes;
    }
public:
    static inline s32 InitSpaceFromConfig(zshm_space_entry& params, bool isUseHeap)
    {
        memset(&params, 0, sizeof(params));
        params.shm_key_ = 198709;
        params.use_heap_ = isUseHeap;
#ifdef WIN32
        
#else
        params.use_fixed_address_ = true;
        params.space_addr_ = 0x00006AAAAAAAAAAAULL;
        params.space_addr_ = 0x0000700000000000ULL;
#endif // WIN32

        params.spaces_[ShmSpace::kFrame].size_ = SPACE_ALIGN(sizeof(Frame));
        params.spaces_[ShmSpace::kBuddy].size_ = SPACE_ALIGN(zbuddy::zbuddy_size(kDynSpaceOrder));
        params.spaces_[ShmSpace::kMalloc].size_ = SPACE_ALIGN(zmalloc::zmalloc_size());
        params.spaces_[ShmSpace::kDyn].size_ = SPACE_ALIGN(zbuddy_shift_size(kDynSpaceOrder + kPageOrder));

        params.whole_space_.size_ += SPACE_ALIGN(sizeof(params));
        for (u32 i = 0; i < ZSHM_MAX_SPACES; i++)
        {
            params.spaces_[i].offset_ = params.whole_space_.size_;
            params.whole_space_.size_ += params.spaces_[i].size_;
        }
        return 0;
    }


    static inline s32 BuildShm(bool isUseHeap)
    {
        zshm_space_entry params;
        InitSpaceFromConfig(params, isUseHeap);
        

        zshm_boot booter;
        zshm_space_entry* shm_space = nullptr;
        s32 ret = booter.build_frame(params, shm_space);
        if (ret != 0 || shm_space == nullptr)
        {
            LogError() << "build_frame error. shm_space:" << (void*)shm_space << ", ret:" << ret;
            return ret;
        }
        ShmInstance() = (char*)shm_space;

        SpaceEntry().space_addr_ = (u64)shm_space;

        BuildObject<Frame>(space<Frame, ShmSpace::kFrame>());
        zbuddy* buddy_ptr = space<zbuddy, ShmSpace::kBuddy>();
        memset(buddy_ptr, 0, params.spaces_[ShmSpace::kBuddy].size_);
        buddy_ptr->set_global(buddy_ptr);
        zbuddy::build_zbuddy(buddy_ptr, params.spaces_[ShmSpace::kBuddy].size_, kDynSpaceOrder, &ret);
        if (ret != 0 )
        {
            LogError() << "";
            return ret;
        }

        zmalloc* malloc_ptr = space<zmalloc, ShmSpace::kMalloc>();
        memset(malloc_ptr, 0, zmalloc::zmalloc_size());
        malloc_ptr->set_global(malloc_ptr);
        malloc_ptr->set_block_callback(&AllocLarge, &FreeLarge);
        malloc_ptr->check_health();


        ret = space<Frame, ShmSpace::kFrame>()->Start();
        if (ret != 0)
        {
            LogError() << "";
            return ret;
        }

        return 0;
    }

    static inline s32 ResumeShm(bool isUseHeap)
    {
        zshm_space_entry params;
        InitSpaceFromConfig(params, isUseHeap);
        zshm_boot booter;
        zshm_space_entry* shm_space = nullptr;
        s32 ret = booter.resume_frame(params, shm_space);
        if (ret != 0 || shm_space == nullptr)
        {
            LogError() << "booter.resume_frame error. shm_space:" << (void*)shm_space <<", ret:" << ret;
            return ret;
        }
        ShmInstance() = (char*)shm_space;

        SpaceEntry().space_addr_ = (u64)shm_space;
        Frame* m = space<Frame, ShmSpace::kFrame>();
        RebuildVPTR<Frame>(m);


        zbuddy* buddy_ptr = space<zbuddy, ShmSpace::kBuddy>();
        buddy_ptr->set_global(buddy_ptr);
        zbuddy::rebuild_zbuddy(buddy_ptr, params.spaces_[ShmSpace::kBuddy].size_, kDynSpaceOrder, &ret);
        if (ret != 0 )
        {
            LogError() << "";
            return ret;
        }

        zmalloc* malloc_ptr = space<zmalloc, ShmSpace::kMalloc>();
        malloc_ptr->set_global(malloc_ptr);
        malloc_ptr->set_block_callback(&AllocLarge, &FreeLarge);
        malloc_ptr->check_health();

        ret = space<Frame, ShmSpace::kFrame>()->Resume();
        if (ret != 0)
        {
            LogError() << "";
            return ret;
        }
        return 0;
    }

    static inline s32 HoldShm(bool isUseHeap)
    {
        if (isUseHeap)
        {
            LogError() << "";
            return -1;
        }
        zshm_space_entry params;
        InitSpaceFromConfig(params, isUseHeap);
        zshm_boot booter;
        zshm_space_entry* shm_space = nullptr;
        s32 ret = booter.resume_frame(params, shm_space);
        if (ret != 0 || shm_space == nullptr)
        {
            LogError() << "";
            return ret;
        }
        ShmInstance() = (char*)shm_space;
        return 0;
    }

    static inline s32 DestroyShm(bool isUseHeap, bool self, bool force)
    {
        s32 ret = 0;

        if (!self)
        {
            if (force)
            {
                s32 ret = HoldShm(isUseHeap);
                if (ret != 0)
                {
                    LogError() << "";
                    return ret;
                }
            }
            else
            {
                s32 ret = ResumeShm(isUseHeap);
                if (ret != 0)
                {
                    LogError() << "";
                    return ret;
                }
            }
        }

        if (ShmInstance() == nullptr)
        {
            LogError() << "";
            return -1;
        }

        if (!force)
        {
            DestroyObject(space<Frame, ShmSpace::kFrame>());
        }

        ret = zshm_boot::destroy_frame(SpaceEntry());
        if (ret != 0)
        {
            LogError() << "";
            return ret;
        }
        return 0;
    }

private:

};


class MyServer : public BaseFrame
{
public:
    s32 Start()
    {
        s32 ret = BaseFrame::Start();
        if (ret != 0)
        {
            LogError() << "error";
            return -1;
        }
        LogInfo() << "MyServer Start";



        return 0;
    }
    s32 Resume()
    {
        s32 ret = BaseFrame::Resume();
        if (ret != 0)
        {
            LogError() << "error";
            return -1;
        }
        LogInfo() << "MyServer Resume";
        return 0;
    }
};








s32 zmalloc_stress()
{
    zmalloc* zstate = zmalloc::instance_ptr();
    static const u32 rand_size = 1000 * 10000;
    static_assert(rand_size > zmalloc::DEFAULT_BLOCK_SIZE, "");
    u32* rand_array = new u32[rand_size];
    static const u32 cover_size = zmalloc::BIG_MAX_REQUEST;
    using Addr = void*;
    zarray <Addr, cover_size>* buffers = new zarray <Addr, cover_size>();
    zarray <Addr, cover_size>* buffers2 = new zarray <Addr, cover_size>();

    //固定小字节申请  
    static int fixed_size = 100;

    for (u32 i = 0; i < zmalloc::DEFAULT_BLOCK_SIZE; i++)
    {
        rand_array[i] = i;
    }
    for (u32 i = zmalloc::DEFAULT_BLOCK_SIZE; i < rand_size; i++)
    {
        rand_array[i] = rand() % (zmalloc::BIG_MAX_REQUEST * 4 / 3);
    }



    PROF_DEFINE_COUNTER(cost);


    PROF_START_COUNTER(cost);
    for (u64 i = 0; i < rand_size; i++)
    {
        global_zfree(global_zmalloc(1));
    }
    PROF_OUTPUT_MULTI_COUNT_CPU("global_zfree(global_zmalloc(1))", rand_size, cost.stop_and_save().cycles());

    PROF_START_COUNTER(cost);
    for (u64 i = 0; i < rand_size; i++)
    {
        u32 test_size = rand_array[i] % (1024);
        void* p = global_zmalloc(test_size);
        global_zfree(p);
    }
    PROF_OUTPUT_MULTI_COUNT_CPU("global_zfree(global_zmalloc(0~1024))", rand_size, cost.stop_and_save().cycles());

    PROF_START_COUNTER(cost);
    for (u64 i = 0; i < rand_size; i++)
    {
        u32 test_size = (rand_array[i] % (zmalloc::BIG_MAX_REQUEST - zmalloc::SMALL_MAX_REQUEST)) + zmalloc::SMALL_MAX_REQUEST;
        void* p = global_zmalloc(test_size);
        global_zfree(p);
    }
    PROF_OUTPUT_MULTI_COUNT_CPU("global_zfree(global_zmalloc(1024~512k))", rand_size, cost.stop_and_save().cycles());

    PROF_START_COUNTER(cost);
    for (u64 i = rand_size / 2; i < rand_size; i++)
    {
        u32 test_size = rand_array[i] % (zmalloc::BIG_MAX_REQUEST * 4 / 3);
        void* p = global_zmalloc(test_size);
        global_zfree(p);
    }
    PROF_OUTPUT_MULTI_COUNT_CPU("global_zfree(global_zmalloc(0~1M))", rand_size, cost.stop_and_save().cycles());

    PROF_START_COUNTER(cost);
    for (u64 i = rand_size / 2; i < rand_size; i++)
    {
        u32 test_size = rand_array[i] % (zmalloc::BIG_MAX_REQUEST * 4 / 3);
        void* p = global_zmalloc(test_size);
        buffers->push_back(p);
        global_zfree(p);
        if (buffers->full())
        {
            buffers->clear();
        }
    }
    PROF_OUTPUT_MULTI_COUNT_CPU("global_zfree(global_zmalloc(0~1M))", rand_size, cost.stop_and_save().cycles());
    buffers->clear();
    PROF_OUTPUT_SELF_MEM("base alloc/free test finish");
    LogInfo() << "";
    LogInfo() << "begin stress test";
    for (size_t loop = 0; loop < 80; loop++)
    {
        u64 begin_size = cover_size / 80 * loop;
        u64 end_size = cover_size / 80 * (loop + 1);
        char mbuf[70];
        sprintf(mbuf, "global_zmalloc(%llu ~ %llu)", begin_size, end_size);
        char fbuf[70];
        sprintf(fbuf, "global_zfree(%llu ~ %llu)", begin_size, end_size);

        PROF_START_COUNTER(cost);
        for (u64 i = begin_size; i < end_size; i++)
        {
            u32 test_size = rand_array[i] % (zmalloc::BIG_MAX_REQUEST);
            void* p = global_zmalloc(test_size);
            *(u32*)p = (u32)i;
            buffers->push_back(p);
        }

        PROF_OUTPUT_MULTI_COUNT_CPU(mbuf, buffers->size(), cost.stop_and_save().cycles());
        if (loop < 2 || loop >37)
        {
            //LogDebug() << zmalloc::instance().debug_color_string();
        }
        PROF_START_COUNTER(cost);
        for (auto p : *buffers)
        {
            global_zfree(p);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU(fbuf, buffers->size(), cost.stop_and_save().cycles());
        buffers->clear();
    }
    zmalloc::instance().clear_cache();
    PROF_OUTPUT_SELF_MEM("zmalloc finish");


    for (size_t loop = 0; loop < 80; loop++)
    {
        if (loop % 10 != 0)
        {
            continue;//做对比用 剔除部分数据提高测试速度
        }
        u64 begin_size = cover_size / 80 * loop;
        u64 end_size = cover_size / 80 * (loop + 1);
        char mbuf[70];
        sprintf(mbuf, "sys malloc(%llu ~ %llu)", begin_size, end_size);
        char fbuf[70];
        sprintf(fbuf, "sys free(%llu ~ %llu)", begin_size, end_size);


        PROF_START_COUNTER(cost);
        for (u64 i = begin_size; i < end_size; i++)
        {
            u32 test_size = rand_array[i] % (zmalloc::BIG_MAX_REQUEST);
            test_size = test_size < 8 ? 8 : test_size;
            void* p = malloc(test_size);
            *(u32*)p = (u32)i;
            buffers->push_back(p);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU(mbuf, buffers->size(), cost.stop_and_save().cycles());
        PROF_START_COUNTER(cost);
        for (auto p : *buffers)
        {
            free(p);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU(fbuf, buffers->size(), cost.stop_and_save().cycles());
        buffers->clear();
    }
    PROF_OUTPUT_SELF_MEM("sys malloc finish");
    
    LogInfo() << "";
    LogInfo() << "begin double list rand malloc&free stress test";
    if (true)
    {
        buffers->clear();
        buffers2->clear();
        PROF_START_COUNTER(cost);
        u64 alloc_count = 0;
        u64 free_count = 0;
        for (u64 i = 0; i < cover_size; i++)
        {
            if (rand() % 5 == 0)
            {
                continue;
            }
            u32 push_size1 = rand_array[i] % (2048);
            u32 push_size2 = rand_array[cover_size - i] % (2048);
            if ((push_size1 + push_size2) % 3 == 0 || buffers->size() > (u32)rand() % 1000 || buffers->full())
            {
                if (!buffers->empty())
                {
                    global_zfree(buffers->back());
                    buffers->pop_back();
                    free_count++;
                }
            }
            if ((push_size1 + push_size2) % 7 == 0 || buffers->size() > (u32)rand() % 1000 || buffers2->full())
            {
                if (!buffers2->empty())
                {
                    global_zfree(buffers2->back());
                    buffers2->pop_back();
                    free_count++;
                }
            }

            buffers->push_back(global_zmalloc(push_size1));
            buffers2->push_back(global_zmalloc(push_size2));
            alloc_count += 2;
        }
        PROF_OUTPUT_MULTI_COUNT_CPU("rand zmalloc/zfree(0~2k)", alloc_count + free_count, cost.stop_and_save().cycles());
        zstate->check_health();
        if (true)
        {
            LogDebug() << "zmalloc state log:";
            auto new_log = []() { return std::move(LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL)); };
            cost.start();
            zmalloc::instance().debug_state_log(new_log);
            zmalloc::instance().debug_color_log(new_log, 0, (zmalloc::CHUNK_COLOR_MASK_WITH_LEVEL + 1) / 2);
            PROF_OUTPUT_SINGLE_CPU("zamlloc debug_state_log debug_color_log cost", cost.stop_and_save().cycles());
        }

        for (auto p : *buffers)
        {
            global_zfree(p);
        }
        for (auto p : *buffers2)
        {
            global_zfree(p);
        }
        buffers->clear();
        buffers2->clear();
        LogDebug() << "zmalloc clear all buffers state log:";
        auto new_log = []() { return std::move(LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL)); };
        zmalloc::instance().debug_state_log(new_log);
        zmalloc::instance().debug_color_log(new_log, 0, (zmalloc::CHUNK_COLOR_MASK_WITH_LEVEL + 1) / 2);
    }
    PROF_OUTPUT_SELF_MEM("z malloc finish");
    if (true)
    {
        buffers->clear();
        buffers2->clear();
        PROF_START_COUNTER(cost);
        u64 alloc_count = 0;
        u64 free_count = 0;
        for (u64 i = 0; i < cover_size; i++)
        {
            if (rand() % 5 == 0)
            {
                continue;
            }
            u32 push_size1 = rand_array[i] % (2048) + 1;
            u32 push_size2 = rand_array[cover_size - i] % (2048) + 1;
            if ((push_size1 + push_size2) % 3 == 0 || buffers->size() > (u32)rand() % 1000 || buffers->full())
            {
                if (!buffers->empty())
                {
                    free(buffers->back());
                    buffers->pop_back();
                    free_count++;
                }
            }
            if ((push_size1 + push_size2) % 7 == 0 || buffers->size() > (u32)rand() % 1000 || buffers2->full())
            {
                if (!buffers2->empty())
                {
                    free(buffers2->back());
                    buffers2->pop_back();
                    free_count++;
                }
            }

            buffers->push_back(malloc(push_size1));
            buffers2->push_back(malloc(push_size2));
            alloc_count += 2;
        }
        PROF_OUTPUT_MULTI_COUNT_CPU("rand sys malloc/free(0~2k)", alloc_count + free_count, cost.stop_and_save().cycles());
        for (auto p : *buffers)
        {
            free(p);
        }
        for (auto p : *buffers2)
        {
            free(p);
        }

        buffers->clear();
        buffers2->clear();
    }
    PROF_OUTPUT_SELF_MEM("sys malloc finish");




    for (size_t loop = 0; loop < 80; loop++)
    {
        u64 begin_size = cover_size / 80 * loop;
        u64 end_size = cover_size / 80 * (loop + 1);
        PROF_START_COUNTER(cost);
        for (u64 i = begin_size; i < end_size; i++)
        {
            u32 test_size = fixed_size;
            void* p = global_zmalloc(test_size);
            buffers->push_back(p);
        }
        cost.stop_and_save();
        char buf[80];
        sprintf(buf, "zmalloc[%llu~%llu) bat", begin_size, end_size);
        PROF_OUTPUT_MULTI_COUNT_CPU(buf, buffers->size(), cost.cycles());
        zstate->check_health();
        PROF_START_COUNTER(cost);
        for (auto p : *buffers)
        {
            global_zfree(p);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU("zfree bat", buffers->size(), cost.stop_and_save().cycles());
        buffers->clear();
        zstate->check_health();
    }
    zstate->clear_cache();
    PROF_OUTPUT_SELF_MEM("z malloc finish");

    for (size_t loop = 0; loop < 80; loop++)
    {
        if (loop % 8 != 0)
        {
            continue;//做对比用 剔除部分数据提高测试速度
        }
        u64 begin_size = cover_size / 80 * loop;
        u64 end_size = cover_size / 80 * (loop + 1);
        PROF_START_COUNTER(cost);
        for (u64 i = begin_size; i < end_size; i++)
        {
            u32 test_size = fixed_size;
            void* p = malloc(test_size);
            buffers->push_back(p);
        }
        cost.stop_and_save();
        char buf[80];
        sprintf(buf, "sys malloc[%llu~%llu) bat", begin_size, end_size);
        PROF_OUTPUT_MULTI_COUNT_CPU(buf, buffers->size(), cost.cycles());

        PROF_START_COUNTER(cost);
        for (auto p : *buffers)
        {
            free(p);
        }
        PROF_OUTPUT_MULTI_COUNT_CPU("sys free bat", buffers->size(), cost.stop_and_save().cycles());
        buffers->clear();
    }
    PROF_OUTPUT_SELF_MEM("sys malloc finish");



    LogDebug() << "check health";
    buffers->clear();

    for (size_t loop = 0; loop < 80; loop++)
    {
        u64 begin_size = cover_size / 80 * loop;
        u64 end_size = cover_size / 80 * (loop + 1);
        for (u64 i = begin_size; i < end_size; i++)
        {
            u32 test_size = rand_array[i] % (zmalloc::BIG_MAX_REQUEST);
            void* p = global_zmalloc(test_size);
            if (test_size < 64)
            {
                memset(p, 0, test_size);
            }
            else
            {
                *(u64*)p = 0;
                *(((u64*)p) + test_size / 8 - 1) = 0;
            }
            buffers->push_back(p);
        }
        zstate->check_health();
        LogDebug() << "check global_zmalloc alloc[" << begin_size << "~" << end_size << ") success";
        for (auto p : *buffers)
        {
            global_zfree(p);
        }
        zstate->check_health();
        LogDebug() << "check global_zmalloc free[" << begin_size << "~" << end_size << ") success";
        buffers->clear();
    }
    void* pz = global_zmalloc(0);
    zstate->check_health();
    global_zfree(pz);


    zstate->clear_cache();
    LogDebug() << "check health finish";
    ASSERT_TEST_EQ(zstate->used_block_count_ + zstate->reserve_block_count_, 0U, "");
    delete[]rand_array;
    return 0;
}



class StressServer : public BaseFrame
{
public:
    s32 Start()
    {
        s32 ret = BaseFrame::Start();
        if (ret != 0)
        {
            LogError() << "error";
            return -1;
        }
        LogInfo() << "MyServer Start";

        ASSERT_TEST(zmalloc_stress() == 0);

        return 0;
    }
    s32 Resume()
    {
        s32 ret = BaseFrame::Resume();
        if (ret != 0)
        {
            LogError() << "error";
            return -1;
        }
        LogInfo() << "MyServer Resume";
        ASSERT_TEST(zmalloc_stress() == 0);
        return 0;
    }
};


s32 boot_base_test(const std::string& option)
{
    bool use_heap = false;
    if (option.find("heap") != std::string::npos)
    {
        use_heap = true;
    }

    if (option.find("stress") != std::string::npos)
    {
        if (option.find("start") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<StressServer>::BuildShm(use_heap) == 0);
        }
        if (option.find("stop") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<StressServer>::DestroyShm(use_heap, false, true) == 0);
        }

        if (option.find("resume") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<StressServer>::ResumeShm(use_heap) == 0);
        }

        if (option.find("hold") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<StressServer>::HoldShm(use_heap) == 0);
        }

    }
    else
    {
        if (option.find("start") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<MyServer>::BuildShm(use_heap) == 0);
        }
        if (option.find("stop") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<MyServer>::DestroyShm(use_heap, false, true) == 0);
        }

        if (option.find("resume") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<MyServer>::ResumeShm(use_heap) == 0);
        }

        if (option.find("hold") != std::string::npos)
        {
            ASSERT_TEST(FrameDelegate<MyServer>::HoldShm(use_heap) == 0);
        }

    }

    return 0;
}


int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    PROF_INIT("zshm_boot");
    PROF_SET_OUTPUT(&FNLogFunc);

    std::string option;
    if (argc <= 1)
    {
        LogInfo() << "used [start stop resume hold] +- [heap] to start server test";
        return 0;
    }
    for (int i = 1; i < argc; i++)
    {
        option += argv[i];
    }
    
    LogInfo() << "option:" << option;

    ASSERT_TEST(boot_base_test(option) == 0);

    LogInfo() << "all test finish .";



    return 0;
}


