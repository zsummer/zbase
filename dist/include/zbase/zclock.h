
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>. 
* All rights reserved
* This file is part of the zbase, used MIT License.   
*/



//from zprof; Copyright one author  
//more test info to view zprof wiki  

#ifndef  ZCLOCK_H
#define ZCLOCK_H

#include <type_traits>
#include <iterator>
#include <cstddef>
#include <memory>
#include <algorithm>

#ifndef ZBASE_SHORT_TYPE
#define ZBASE_SHORT_TYPE
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
        kClockNull,
        kClockSys,
        kClockClock,
        kClockChrono,
        kClockSteadyChrono,
        kClockSysChrono,

        kClockPureRDTSC,
        kClockVolatileRDTSC,
        kClockFenceRDTSC,
        kClockBTBFenceRDTSC,
        kClockRDTSCP,
        kClockMFenceRDTSC,
        kClockBTBMFenceRDTSC,

        kClockLockRDTSC,
        kClock_MAX,
    };


    struct vmdata
    {
        u64 vm_size;
        u64 rss_size;
        u64 shr_size;
    };

    template<clock_type _Ty>
    s64 get_clock()
    {
        return 0;
    }

    template<>
    s64 get_clock<kClockFenceRDTSC>()
    {
#ifdef WIN32
        _mm_lfence();
        return (s64)__rdtsc();
#else
        u32 lo, hi;
        __asm__ __volatile__("lfence;rdtsc" : "=a" (lo), "=d" (hi) ::);
        u64 val = ((u64)hi << 32) | lo;
        return (s64)val;
#endif
    }

    template<>
    s64 get_clock<kClockBTBFenceRDTSC>()
    {
#ifdef WIN32
        s64 ret;
        _mm_lfence();
        ret = (s64)__rdtsc();
        _mm_lfence();
        return ret;
#else
        u32 lo, hi;
        __asm__ __volatile__("lfence;rdtsc;lfence" : "=a" (lo), "=d" (hi) ::);
        u64 val = ((u64)hi << 32) | lo;
        return (s64)val;
#endif
    }


    template<>
    s64 get_clock<kClockVolatileRDTSC>()
    {
#ifdef WIN32
        return (s64)__rdtsc();
#else
        unsigned long hi, lo;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi) ::);
        u64 val = (((u64)hi) << 32 | ((u64)lo));
        return (s64)val;
#endif
    }

    template<>
    s64 get_clock<kClockPureRDTSC>()
    {
#ifdef WIN32
        return (s64)__rdtsc();
#else
        unsigned long hi, lo;
        __asm__("rdtsc" : "=a"(lo), "=d"(hi));
        u64 val = (((u64)hi) << 32 | ((u64)lo));
        return (s64)val;
#endif
    }

    template<>
    s64 get_clock<kClockLockRDTSC>()
    {
#ifdef WIN32
        _mm_mfence();
        return (s64)__rdtsc();
#else
        unsigned long hi, lo;
        __asm__("lock addq $0, 0(%%rsp); rdtsc" : "=a"(lo), "=d"(hi)::"memory");
        u64 val = (((u64)hi) << 32 | ((u64)lo));
        return (s64)val;
#endif
    }


    template<>
    s64 get_clock<kClockMFenceRDTSC>()
    {
#ifdef WIN32
        s64 ret = 0;
        _mm_mfence();
        ret = (s64)__rdtsc();
        _mm_mfence();
        return ret;
#else
        unsigned long lo, hi;
        __asm__ __volatile__("mfence;rdtsc;mfence" : "=a" (lo), "=d" (hi) ::);
        u64 val = ((u64)hi << 32) | lo;
        return (s64)val;
#endif
    }

    template<>
    s64 get_clock<kClockBTBMFenceRDTSC>()
    {
#ifdef WIN32
        _mm_mfence();
        return (s64)__rdtsc();
#else
        unsigned long lo, hi;
        __asm__ __volatile__("mfence;rdtsc" : "=a" (lo), "=d" (hi) :: "memory");
        u64 val = ((u64)hi << 32) | lo;
        return (s64)val;
#endif
    }

    template<>
    s64 get_clock<kClockRDTSCP>()
    {
#ifdef WIN32
        u32 ui;
        return (s64)__rdtscp(&ui);
#else
        unsigned long hi, lo;
        __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi)::"memory");
        u64 val = (((u64)hi) << 32 | ((u64)lo));
        return (s64)val;
#endif
    }


    template<>
    s64 get_clock<kClockClock>()
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
    s64 get_clock<kClockSys>()
    {
#if (defined WIN32)
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        u64 tsc = ft.dwHighDateTime;
        tsc <<= 32;
        tsc |= ft.dwLowDateTime;
        tsc /= 10;
        tsc -= 11644473600000000ULL;
        return (s64)tsc * 1000; //ns
#else
        struct timeval tm;
        gettimeofday(&tm, nullptr);
        return tm.tv_sec * 1000 * 1000 * 1000 + tm.tv_usec * 1000;
#endif
    }

    template<>
    s64 get_clock<kClockChrono>()
    {
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
    }

    template<>
    s64 get_clock<kClockSteadyChrono>()
    {
        return std::chrono::steady_clock().now().time_since_epoch().count();
    }

    template<>
    s64 get_clock<kClockSysChrono>()
    {
        return std::chrono::system_clock().now().time_since_epoch().count();
    }


    inline vmdata get_vmdata()
    {
        vmdata vm = { 0ULL, 0ULL, 0ULL };
#ifdef WIN32
        HANDLE hproc = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hproc, &pmc, sizeof(pmc)))
        {
            CloseHandle(hproc);// ignore  
            vm.vm_size = (u64)pmc.WorkingSetSize;
            vm.rss_size = (u64)pmc.WorkingSetSize;
        }
#else
        const char* file = "/proc/self/statm";
        FILE* fp = fopen(file, "r");
        if (fp != NULL)
        {
            char line_buff[256];
            while (fgets(line_buff, sizeof(line_buff), fp) != NULL)
            {
                s32 ret = sscanf(line_buff, "%lld %lld %lld ", &vm.vm_size, &vm.rss_size, &vm.shr_size);
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
    inline double get_cpu_freq()
    {
        double mhz = 1;
#ifdef __APPLE__
        s32 mib[2];
        u32 freq;
        size_t len;
        mib[0] = CTL_HW;
        mib[1] = HW_CPU_FREQ;
        len = sizeof(freq);
        sysctl(mib, 2, &freq, &len, NULL, 0);
        mhz = freq;
        mhz /= 1000.0 * 1000.0;
#elif (defined WIN32)
        SYSTEM_INFO si = { 0 };
        GetSystemInfo(&si);
        std::array< PROF_PROCESSOR_POWER_INFORMATION, 128> pppi;
        DWORD dwSize = sizeof(PROF_PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors;
        memset(&pppi[0], 0, dwSize);
        long ret = CallNtPowerInformation(ProcessorInformation, NULL, 0, &pppi[0], dwSize);
        if (ret != 0 || pppi[0].MaxMhz <= 0)
        {
            return 1;
        }
        mhz = pppi[0].MaxMhz;
#else
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

                s32 ret = sscanf(p, "%lf", &mhz);
                if (ret <= 0)
                {
                    mhz = 1;
                    break;
                }
                break;
            }
        }
        fclose(fp);
#endif // __APPLE__
        return mhz;
    }




    template<clock_type _Ty>
    double get_frequency()
    {
        return 1.0;
    }

    template<>
    double get_frequency<kClockFenceRDTSC>()
    {
        const static double frequency_per_ns = get_cpu_freq() * 1000.0 * 1000.0 / 1000.0 / 1000.0 / 1000.0;
        return frequency_per_ns;
    }
    template<>
    double get_frequency<kClockBTBFenceRDTSC>()
    {
        return get_frequency<kClockFenceRDTSC>();
    }

    template<>
    double get_frequency<kClockVolatileRDTSC>()
    {
        return get_frequency<kClockFenceRDTSC>();
    }

    template<>
    double get_frequency<kClockPureRDTSC>()
    {
        return get_frequency<kClockFenceRDTSC>();
    }

    template<>
    double get_frequency<kClockLockRDTSC>()
    {
        return get_frequency<kClockFenceRDTSC>();
    }

    template<>
    double get_frequency<kClockMFenceRDTSC>()
    {
        return get_frequency<kClockFenceRDTSC>();
    }

    template<>
    double get_frequency<kClockBTBMFenceRDTSC>()
    {
        return get_frequency<kClockFenceRDTSC>();
    }

    template<>
    double get_frequency<kClockRDTSCP>()
    {
        return get_frequency<kClockFenceRDTSC>();
    }

    template<>
    double get_frequency<kClockClock>()
    {
#ifdef WIN32
        double frequency_per_ns = 0;
        LARGE_INTEGER win_freq;
        win_freq.QuadPart = 0;
        QueryPerformanceFrequency((LARGE_INTEGER*)&win_freq);
        frequency_per_ns = win_freq.QuadPart / 1000.0 / 1000.0 / 1000.0;
        return frequency_per_ns;
#else
        return 1.0;
#endif
    }

    template<>
    double get_frequency<kClockSys>()
    {
        return 1.0;
    }

    template<>
    double get_frequency<kClockChrono>()
    {
        const static double chrono_frequency = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<>
    double get_frequency<kClockSteadyChrono>()
    {
        const static double chrono_frequency = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<>
    double get_frequency<kClockSysChrono>()
    {
        const static double chrono_frequency = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<clock_type _Ty>
    double get_inverse_frequency()
    {
        const static double inverse_frequency_per_ns = 1.0 / (get_frequency<_Ty>() <= 0.0 ? 1.0 : get_frequency<_Ty>());
        return inverse_frequency_per_ns;
    }
}




template<zclock_impl::clock_type _Ty = zclock_impl::kClockVolatileRDTSC>
class zclock_base
{
private:
    s64 start_clock_;
    s64 cycles_;
public:
    s64 get_started_clock() const { return start_clock_; }
    s64 get_cycles() const { return cycles_; }
    s64 get_stopped_clock() const { return start_clock_ + cycles_; }

    void set_start_clock(s64 val) { start_clock_ = val; }
    void set_cycles(s64 cycles) { cycles_ = cycles; }
public:
    zclock_base()
    {
        start_clock_ = 0;
        cycles_ = 0;
    }
    zclock_base(s64 start_clock)
    {
        start_clock_ = start_clock;
        cycles_ = 0;
    }
    zclock_base(const zclock_base& c)
    {
        start_clock_ = c.start_clock_;
        cycles_ = c.cycles_;
    }
    void start()
    {
        start_clock_ = zclock_impl::get_clock<_Ty>();
        cycles_ = 0;
    }

    zclock_base& save()
    {
        cycles_ = zclock_impl::get_clock<_Ty>() - start_clock_;
        return *this;
    }

    zclock_base& stop_and_save() { return save(); }


    s64 duration_cycles()const { return cycles_; }
    s64 duration_ns()const { return (s64)(cycles_ * zclock_impl::get_inverse_frequency<_Ty>()); }
    s64 duration_ms()const { return duration_ns() / 1000 / 1000; }
    double duration_second() const { return (double)duration_ns() / (1000.0 * 1000.0 * 1000.0); }

    //utils
public:
    static s64 now_ms() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
    static double now() { return std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count(); }
    static zclock_impl::vmdata get_vmdata() { return zclock_impl::get_vmdata(); }
};


using zclock = zclock_base<>;













#endif