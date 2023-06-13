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

using shm_header = zarray<u32, 100>;

static constexpr u32 kPageOrder = 20; //1m  
static constexpr u32 kDynSpaceOrder = 10; // 1024  

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
        T* inst = new T(args...);
        *(u64*)addr = *(u64*)inst;
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
public:
    static inline s32 InitSpaceFromConfig(zshm_space_entry& params, bool isUseHeap)
    {
        memset(&params, 0, sizeof(params));
        params.shm_key_ = 198709;
        params.use_heap_ = isUseHeap;
        params.spaces_[ShmSpace::kFrame].size_ = SPACE_ALIGN(sizeof(Frame));
        params.spaces_[ShmSpace::kBuddy].size_ = SPACE_ALIGN(zbuddy::get_zbuddy_head_size(kDynSpaceOrder));
        params.spaces_[ShmSpace::kMalloc].size_ = SPACE_ALIGN(sizeof(zmalloc));
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
            LogError() << "";
            return ret;
        }
        ShmInstance() = (char*)shm_space;

        SpaceEntry().space_addr_ = (u64)shm_space;

        BuildObject<Frame>(space<Frame, ShmSpace::kFrame>());
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
            LogError() << "";
            return ret;
        }
        ShmInstance() = (char*)shm_space;

        SpaceEntry().space_addr_ = (u64)shm_space;
        Frame* m = space<Frame, ShmSpace::kFrame>();
        RebuildVPTR<Frame>(m);
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
        return 0;
    }

    static inline s32 ReleaseShm(bool isUseHeap)
    {
        DestroyObject(space<Frame, ShmSpace::kFrame>());
        s32 ret = zshm_boot::destroy_frame(SpaceEntry());
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





s32 shm_loader_base_test(const std::string& option)
{
    bool use_heap = false;
    if (option.find("heap") != std::string::npos)
    {
        use_heap = true;
    }

    if (option.find("start") != std::string::npos)
    {
        ASSERT_TEST(FrameDelegate<MyServer>::BuildShm(use_heap) == 0);
    }
    if (option.find("stop") != std::string::npos)
    {
        ASSERT_TEST(FrameDelegate<MyServer>::ResumeShm(use_heap) == 0);
        ASSERT_TEST(FrameDelegate<MyServer>::ReleaseShm(use_heap) == 0);
    }

    if (option.find("resume") != std::string::npos)
    {
        ASSERT_TEST(FrameDelegate<MyServer>::ResumeShm(use_heap) == 0);
    }

    if (option.find("hold") != std::string::npos)
    {
        ASSERT_TEST(FrameDelegate<MyServer>::HoldShm(use_heap) == 0);
    }

    return 0;
}






int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    PROF_INIT("shm_loader");
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

    ASSERT_TEST(shm_loader_base_test(option) == 0);

    LogInfo() << "all test finish .";

#ifdef WIN32
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
#endif // WIN32

    return 0;
}


