
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/



//from zprof; Copyright one author  
//more test info to view zprof wiki  


#pragma once 
#ifndef  ZCLOCK_H
#define ZCLOCK_H

#include <stdint.h>
#include <type_traits>
#include <iterator>
#include <cstddef>
#include <memory>
#include <algorithm>


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
* is_trivially_copyable: yes
    * memset: yes
    * memcpy: yes
* shm resume : safely
    * has vptr:     no
    * static var:   no(has const static val)
    * has heap ptr: no
    * has code ptr: no
    * has sys ptr:  no
* thread safe: safely
*
*/



#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <array>
#include <limits.h>
#include <chrono>
#include <string.h>
#ifdef _WIN32
#ifndef KEEP_INPUT_QUICK_EDIT
#define KEEP_INPUT_QUICK_EDIT false
#endif

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <io.h>
#include <shlwapi.h>
#include <process.h>
#include <psapi.h>
#include <powerbase.h>
#include <powrprof.h>
#include <profileapi.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "Kernel32")
#pragma comment(lib, "User32.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"PowrProf.lib")
#pragma warning(disable:4996)

#else
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#
#endif


#ifdef __APPLE__
#include "TargetConditionals.h"
#include <dispatch/dispatch.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#if !TARGET_OS_IPHONE
#define NFLOG_HAVE_LIBPROC
#include <libproc.h>
#endif
#endif

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif



namespace zclock_impl
{
    enum clock_type
    {
        kClockNULL,
        kClockSystem,
        kClockClock,
        kClockChrono,
        kClockSteadyChrono,
        kClockSystemChrono,
        kClockSystemMS, //wall clock 

        kClockPureRDTSC,
        kClockVolatileRDTSC,
        kClockFenceRDTSC,
        kClockMFenceRDTSC,
        kClockLockRDTSC,
        kClockRDTSCP,
        kClockBTBFenceRDTSC,
        kClockBTBMFenceRDTSC,

        kClockMAX,
    };

    
    struct vmdata
    {
        //don't use u64 or long long; uint64_t maybe is long ;
        unsigned long long vm_size;
        unsigned long long rss_size;
        unsigned long long shr_size;
    };

    template<clock_type _C>
    inline long long get_tick()
    {
        return 0;
    }

    template<>
    inline long long get_tick<kClockFenceRDTSC>()
    {
#ifdef WIN32
        _mm_lfence();
        return (long long)__rdtsc();
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(ZCLOCK_NO_RDTSC)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("lfence;rdtsc" : "=a" (lo), "=d" (hi) ::);
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }

    template<>
    inline long long get_tick<kClockBTBFenceRDTSC>()
    {
#ifdef WIN32
        long long ret = 0;
        _mm_lfence();
        ret = (long long)__rdtsc();
        _mm_lfence();
        return ret;
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(ZCLOCK_NO_RDTSC)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("lfence;rdtsc;lfence" : "=a" (lo), "=d" (hi) ::);
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }


    template<>
    inline long long get_tick<kClockVolatileRDTSC>()
    {
#ifdef WIN32
        return (long long)__rdtsc();
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)  && !defined(ZCLOCK_NO_RDTSC)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi) ::);
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
}

    template<>
    inline long long get_tick<kClockPureRDTSC>()
    {
#ifdef WIN32
        return (long long)__rdtsc();
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(ZCLOCK_NO_RDTSC)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));  //pure need __volatile__ too  . 
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }

    template<>
    inline long long get_tick<kClockLockRDTSC>()
    {
#ifdef WIN32
        _mm_mfence();
        return (long long)__rdtsc();
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(ZCLOCK_NO_RDTSC)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__("lock addq $0, 0(%%rsp); rdtsc" : "=a"(lo), "=d"(hi)::"memory");
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }


    template<>
    inline long long get_tick<kClockMFenceRDTSC>()
    {
#ifdef WIN32
        long long ret = 0;
        _mm_mfence();
        ret = (long long)__rdtsc();
        _mm_mfence();
        return ret;
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(ZCLOCK_NO_RDTSC)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("mfence;rdtsc;mfence" : "=a" (lo), "=d" (hi) ::);
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }

    template<>
    inline long long get_tick<kClockBTBMFenceRDTSC>()
    {
#ifdef WIN32
        _mm_mfence();
        return (long long)__rdtsc();
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(ZCLOCK_NO_RDTSC)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("mfence;rdtsc" : "=a" (lo), "=d" (hi) :: "memory");
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }

    template<>
    inline long long get_tick<kClockRDTSCP>()
    {
#ifdef WIN32
        unsigned int ui = 0;
        return (long long)__rdtscp(&ui);
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(ZCLOCK_NO_RDTSC)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi)::"memory");
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }


    template<>
    inline long long get_tick<kClockClock>()
    {
#if (defined WIN32)
        LARGE_INTEGER win_freq;
        win_freq.QuadPart = 0;
        QueryPerformanceCounter((LARGE_INTEGER*)&win_freq);
        return win_freq.QuadPart;
#else
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
#endif
    }

    template<>
    inline long long get_tick<kClockSystem>()
    {
#if (defined WIN32)
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        unsigned long long tsc = ft.dwHighDateTime;
        tsc <<= 32;
        tsc |= ft.dwLowDateTime;
        tsc /= 10;
        tsc -= 11644473600000000ULL;
        return (long long)tsc * 1000; //ns
#else
        struct timeval tm;
        gettimeofday(&tm, nullptr);
        return tm.tv_sec * 1000 * 1000 * 1000 + tm.tv_usec * 1000;
#endif
    }

    template<>
    inline long long get_tick<kClockChrono>()
    {
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
    }

    template<>
    inline long long get_tick<kClockSteadyChrono>()
    {
        return std::chrono::steady_clock().now().time_since_epoch().count();
    }

    template<>
    inline long long get_tick<kClockSystemChrono>()
    {
        return std::chrono::system_clock().now().time_since_epoch().count();
    }

    template<>
    inline long long get_tick<kClockSystemMS>()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }


    inline vmdata get_self_mem()
    {
        vmdata vm = { 0ULL, 0ULL, 0ULL };
#ifdef WIN32
        HANDLE hproc = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hproc, &pmc, sizeof(pmc)))
        {
            CloseHandle(hproc);// ignore  
            vm.vm_size = (unsigned long long)pmc.WorkingSetSize;
            vm.rss_size = (unsigned long long)pmc.WorkingSetSize;
        }
#else
        const char* file = "/proc/self/statm";
        FILE* fp = fopen(file, "r");
        if (fp != NULL)
        {
            char line_buff[256];
            while (fgets(line_buff, sizeof(line_buff), fp) != NULL)
            {
                int ret = sscanf(line_buff, "%lld %lld %lld ", &vm.vm_size, &vm.rss_size, &vm.shr_size);
                if (ret == 3)
                {
                    vm.vm_size *= 4096;
                    vm.rss_size *= 4096;
                    vm.shr_size *= 4096;
                    break;
                }
                memset(&vm, 0, sizeof(vm));
                break;
            }
            fclose(fp);
        }
#endif
        return vm;
    }

    inline vmdata get_sys_mem()
    {
        vmdata vm = { 0ULL, 0ULL, 0ULL };
#ifdef WIN32
        MEMORYSTATUS state = { 0 };
        GlobalMemoryStatus(&state);

        vm.vm_size = (unsigned long long)state.dwTotalPhys;
        vm.rss_size = (unsigned long long)(state.dwTotalPhys - state.dwAvailPhys);
#else
        const char* file = "/proc/meminfo";
        FILE* fp = fopen(file, "r");
        if (fp != NULL)
        {
            char line_buff[256];
            while (fgets(line_buff, sizeof(line_buff), fp) != NULL)
            {
                std::string line = line_buff;
                //std::transform(line.begin(), line.end(), line.begin(), ::toupper);
                if (line.compare(0, strlen("MemTotal:"), "MemTotal:", strlen("MemTotal:")) == 0)
                {
                    int ret = sscanf(line.c_str() + strlen("MemTotal:"), "%lld", &vm.vm_size);
                    if (ret == 1)
                    {
                        vm.vm_size *= 1024;
                    }
                }

                if (line.compare(0, strlen("MemFree:"), "MemFree:", strlen("MemFree:")) == 0)
                {
                    int ret = sscanf(line.c_str() + strlen("MemFree:"), "%lld", &vm.rss_size);
                    if (ret == 1)
                    {
                        vm.rss_size *= 1024;
                    }
                }

                if (line.compare(0, strlen("Cached:"), "Cached:", strlen("Cached:")) == 0)
                {
                    int ret = sscanf(line.c_str() + strlen("Cached:"), "%lld", &vm.shr_size);
                    if (ret == 1)
                    {
                        vm.shr_size *= 1024;
                    }
                }
                if (vm.rss_size != 0 && vm.shr_size != 0 && vm.vm_size != 0)
                {
                    vm.rss_size = vm.vm_size - (vm.rss_size + vm.shr_size);
                    vm.shr_size = 0;
                    break;
                }

            }
            fclose(fp);
        }
#endif
        return vm;
    }

#ifdef WIN32
    struct PROF_PROCESSOR_POWER_INFORMATION
    {
        ULONG  Number;
        ULONG  MaxMhz;
        ULONG  CurrentMhz;
        ULONG  MhzLimit;
        ULONG  MaxIdleState;
        ULONG  CurrentIdleState;
    };
#endif


    // U: nanosecond   
    inline double get_cpu_period()
    {
        double period = 0;
        double ns_per_second = 1000 * 1000 * 1000;

#ifdef ZCLOCK_NO_RDTSC //apple m1 | some amd chip 
        period = ns_per_second * std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
#elif (defined WIN32)
        SYSTEM_INFO si = { 0 };
        GetSystemInfo(&si);
        std::array< PROF_PROCESSOR_POWER_INFORMATION, 128> pppi;
        DWORD dwSize = sizeof(PROF_PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors;
        memset(&pppi[0], 0, dwSize);
        long ret = CallNtPowerInformation(ProcessorInformation, NULL, 0, &pppi[0], dwSize);
        if (ret != 0 || pppi[0].MaxMhz <= 0)
        {
            return 0;
        }
        period = pppi[0].MaxMhz;
        period *= 1000 * 1000;  //mhz --> hz 
        period = ns_per_second / period;
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)  && defined(__APPLE__)
        int mib[2];
        unsigned int freq;
        size_t len;
        mib[0] = CTL_HW;
        mib[1] = HW_CPU_FREQ;
        len = sizeof(freq);
        sysctl(mib, 2, &freq, &len, NULL, 0);
        if (freq == 0) //error
        {
            return 0;
        }
        period = ns_per_second / freq;
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)  //linux 
        const char* file = "/proc/cpuinfo";
        FILE* fp = fopen(file, "r");
        if (NULL == fp)
        {
            return 0;
        }

        char line_buff[256];

        while (fgets(line_buff, sizeof(line_buff), fp) != NULL)
        {
            if (strstr(line_buff, "cpu MHz") != NULL)
            {
                const char* p = line_buff;
                while (p < &line_buff[255] && (*p < '0' || *p > '9'))
                {
                    p++;
                }
                if (p == &line_buff[255])
                {
                    break;
                }

                int ret = sscanf(p, "%lf", &period);
                if (ret <= 0)
                {
                    period = 0;
                    break;
                }
                break;
            }
        }
        fclose(fp);
        period *= 1000 * 1000; //mhz --> hz 
        if (period < 1)
        {
            return 0;
        }
        period = ns_per_second / period;
#else
        period = ns_per_second * std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
#endif // 
        return period;
    }


    template<clock_type _C>
    inline double get_clock_period()
    {
        return 1.0;  //1 ns
    }

    template<>
    inline double get_clock_period<kClockFenceRDTSC>()
    {
        const static double period = get_cpu_period();
        return period;
    }
    template<>
    inline double get_clock_period<kClockBTBFenceRDTSC>()
    {
        return get_clock_period<kClockFenceRDTSC>();
    }

    template<>
    inline double get_clock_period<kClockVolatileRDTSC>()
    {
        return get_clock_period<kClockFenceRDTSC>();
    }

    template<>
    inline double get_clock_period<kClockPureRDTSC>()
    {
        return get_clock_period<kClockFenceRDTSC>();
    }

    template<>
    inline double get_clock_period<kClockLockRDTSC>()
    {
        return get_clock_period<kClockFenceRDTSC>();
    }

    template<>
    inline double get_clock_period<kClockMFenceRDTSC>()
    {
        return get_clock_period<kClockFenceRDTSC>();
    }

    template<>
    inline double get_clock_period<kClockBTBMFenceRDTSC>()
    {
        return get_clock_period<kClockFenceRDTSC>();
    }

    template<>
    inline double get_clock_period<kClockRDTSCP>()
    {
        return get_clock_period<kClockFenceRDTSC>();
    }

    template<>
    inline double get_clock_period<kClockClock>()
    {
#ifdef WIN32
        static double period = 0.0;
        if (period == 0.0)
        {
            LARGE_INTEGER win_freq;
            win_freq.QuadPart = 0;
            QueryPerformanceFrequency((LARGE_INTEGER*)&win_freq);
            if (win_freq.QuadPart == 0)
            {
                win_freq.QuadPart = 1;
            }
            period = 1000.0 * 1000.0 * 1000.0 / win_freq.QuadPart;
        }
        return period;
#else
        return 1.0; //1 ns
#endif
    }

    template<>
    inline double get_clock_period<kClockSystem>()
    {
        return 1.0; //1 ns
    }

    template<>
    inline double get_clock_period<kClockChrono>()
    {
        return 1000.0 * 1000.0 * 1000.0 * std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
    }

    template<>
    inline double get_clock_period<kClockSteadyChrono>()
    {
        return 1000.0 * 1000.0 * 1000.0 * std::chrono::steady_clock::period().num / std::chrono::steady_clock::period().den;
    }

    template<>
    inline double get_clock_period<kClockSystemChrono>()
    {
        return 1000.0 * 1000.0 * 1000.0 * std::chrono::system_clock::period().num / std::chrono::system_clock::period().den;
    }

    template<>
    inline double get_clock_period<kClockSystemMS>()
    {
        return 1000.0 * 1000.0 * 1000.0 / 1000;
    }




    template<clock_type _C = kClockVolatileRDTSC>
    class zclock_base
    {
    public:
        static constexpr clock_type C = _C;

    private:
        long long begin_;
        long long ticks_;
    public:
        long long get_begin() const { return begin_; }
        long long get_ticks() const { return ticks_; }
        long long get_end() const { return begin_ + ticks_; }

        void set_begin(long long val) { begin_ = val; }
        void set_ticks(long long ticks) { ticks_ = ticks; }
    public:
        zclock_base()
        {
            begin_ = 0;
            ticks_ = 0;
        }
        zclock_base(long long start_clock)
        {
            begin_ = start_clock;
            ticks_ = 0;
        }
        zclock_base(const zclock_base& c)
        {
            begin_ = c.begin_;
            ticks_ = c.ticks_;
        }
        void start()
        {
            begin_ = get_tick<_C>();
            ticks_ = 0;
        }

        zclock_base& save()
        {
            ticks_ = get_tick<_C>() - begin_;
            return *this;
        }

        zclock_base& stop_and_save() { return save(); }


        long long cost()const { return ticks_; }
        long long ticks()const { return ticks_; }
        long long cycles()const { return ticks_; }
        long long cost_ns()const { return (long long)(ticks_ * get_clock_period<_C>()); }
        long long cost_ms()const { return cost_ns() / 1000 / 1000; }
        double cost_s() const { return (double)cost_ns() / (1000.0 * 1000.0 * 1000.0); }

        //utils  
    public:
        static long long now() { return get_tick<_C>(); }
        static long long sys_now_ns() { return get_tick<kClockSystem>(); }
        static long long sys_now_us() { return get_tick<kClockSystem>() / 1000; }
        static long long sys_now_ms() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
        static double sys_now_s() { return std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count(); }
        static vmdata get_self_mem() { return get_self_mem(); }
        static vmdata get_sys_mem() { return get_sys_mem(); }
    };
}

template<zclock_impl::clock_type _C = zclock_impl::kClockVolatileRDTSC>
using zclock = zclock_impl::zclock_base<_C>;













#endif