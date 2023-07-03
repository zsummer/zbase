
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>. 
* All rights reserved
* This file is part of the zbase, used MIT License.   
*/




#ifndef  ZSTREAM_H
#define ZSTREAM_H

#include <type_traits>
#include <iterator>
#include <cstddef>
#include <memory>
#include <algorithm>
#include <stdio.h>
#include <stdarg.h>
#include <cmath>
#include <time.h>
#include <string>

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
* is_trivially_copyable: in part
    * memset: uninit or no attach any array_data;
    * memcpy: uninit or no attach any array_data;
* shm resume : safely, require heap address fixed
    * has vptr:     no
    * static var:   no
    * has heap ptr: yes  (attach memory)
    * has code ptr: no
    * has sys ptr:  no
* thread safe: read safe
*
*/

//from fn-log; Copyright one author  
//more test info to view fn-log wiki  

namespace zstream_impl
{
    //__attribute__((format(printf, 3, 4))) 
    template<typename ... Args>
    static inline s32 write_fmt(char* dst, s32 dstlen, const char* fmt_string, Args&& ... args)
    {
        if (dstlen <= 0)
        {
            return 0;
        }

#ifdef WIN32
        int ret = _snprintf_s(dst, dstlen, _TRUNCATE, fmt_string, args ...);
#else
        int ret = snprintf(dst, dstlen, fmt_string, args ...);
#endif // WIN32

        if (ret < 0)
        {
            return 0;
        }
        return ret;
    }

    template<int WIDE>
    static inline int write_ull(char* dst, s32 dst_len, unsigned long long number)
    {
        if (dst_len < WIDE)
        {
            return 0;
        }
        static const char* dec_lut =
            "00010203040506070809"
            "10111213141516171819"
            "20212223242526272829"
            "30313233343536373839"
            "40414243444546474849"
            "50515253545556575859"
            "60616263646566676869"
            "70717273747576777879"
            "80818283848586878889"
            "90919293949596979899";
        static const int buf_len = 30;
        char buf[buf_len];
        int write_index = buf_len;
        unsigned long long m1 = 0;
        unsigned long long m2 = 0;
        do
        {
            m1 = number / 100;
            m2 = number % 100;
            m2 += m2;
            number = m1;
            *(buf + write_index - 1) = dec_lut[m2 + 1];
            *(buf + write_index - 2) = dec_lut[m2];
            write_index -= 2;
        } while (number);
        if (buf[write_index] == '0')
        {
            write_index++;
        }
        while (buf_len - write_index < WIDE)
        {
            write_index--;
            buf[write_index] = '0';
        }

        s32 count = buf_len - write_index;
        if (dst_len < count + 1)
        {
            return 0;
        }
        memcpy(dst, buf + write_index, count);
        *(dst + count) = '\0';
        return count;
    }


    template<int WIDE>
    static inline int write_hex(char* dst, s32 dst_len, unsigned long long number)
    {
        static const char* lut =
            "0123456789ABCDEFGHI";
        static const char* hex_lut =
            "000102030405060708090A0B0C0D0E0F"
            "101112131415161718191A1B1C1D1E1F"
            "202122232425262728292A2B2C2D2E2F"
            "303132333435363738393A3B3C3D3E3F"
            "404142434445464748494A4B4C4D4E4F"
            "505152535455565758595A5B5C5D5E5F"
            "606162636465666768696A6B6C6D6E6F"
            "707172737475767778797A7B7C7D7E7F"
            "808182838485868788898A8B8C8D8E8F"
            "909192939495969798999A9B9C9D9E9F"
            "A0A1A2A3A4A5A6A7A8A9AAABACADAEAF"
            "B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
            "C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF"
            "D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
            "E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
            "F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";

        if (dst_len <16+3 || dst_len < WIDE)
        {
            return 0;
        }

        int real_wide = 0;
#ifndef WIN32
        real_wide = sizeof(number) * 8 - __builtin_clzll(number);
#else
        unsigned long win_index = 0;
        if (_BitScanReverse(&win_index, (unsigned long)(number >> 32)))
        {
            real_wide = win_index + 1 + 32;
        }
        else if (_BitScanReverse(&win_index, (unsigned long)(number & 0xffffffff)))
        {
            real_wide = win_index + 1;
        }
#endif 
        switch (real_wide)
        {
        case  1:case  2:case  3:case  4:real_wide = 1; break;
        case  5:case  6:case  7:case  8:real_wide = 2; break;
        case 9: case 10:case 11:case 12:real_wide = 3; break;
        case 13:case 14:case 15:case 16:real_wide = 4; break;
        case 17:case 18:case 19:case 20:real_wide = 5; break;
        case 21:case 22:case 23:case 24:real_wide = 6; break;
        case 25:case 26:case 27:case 28:real_wide = 7; break;
        case 29:case 30:case 31:case 32:real_wide = 8; break;
        case 33:case 34:case 35:case 36:real_wide = 9; break;
        case 37:case 38:case 39:case 40:real_wide = 10; break;
        case 41:case 42:case 43:case 44:real_wide = 11; break;
        case 45:case 46:case 47:case 48:real_wide = 12; break;
        case 49:case 50:case 51:case 52:real_wide = 13; break;
        case 53:case 54:case 55:case 56:real_wide = 14; break;
        case 57:case 58:case 59:case 60:real_wide = 15; break;
        case 61:case 62:case 63:case 64:real_wide = 16; break;
        }
        if (real_wide < WIDE)
        {
            real_wide = WIDE;
        }
        unsigned long long cur_wide = real_wide;
        while (number && cur_wide >= 2)
        {
            const unsigned long long m2 = (unsigned long long)((number % 256) * 2);
            number /= 256;
            *(dst + cur_wide - 1) = hex_lut[m2 + 1];
            *(dst + cur_wide - 2) = hex_lut[m2];
            cur_wide -= 2;
        }
        if (number)
        {
            *dst = lut[number % 16];
            cur_wide--;
        }
        while (cur_wide-- != 0)
        {
            *(dst + cur_wide) = '0';
        }
        *(dst + real_wide) = '\0';
        return real_wide;
    }
    template<int WIDE>
    static inline int write_ll(char* dst, s32 dst_len, long long number)
    {
        if (dst_len < 2)
        {
            return 0;
        }
        if (number < 0)
        {
            *dst = '-';
            return 1 + write_ull < (WIDE > 0) ? (WIDE - 1) : (0) > (dst + 1, dst_len - 1, (u64)abs(number));
        }
        return write_ull<WIDE>(dst, dst_len, (u64)abs(number));
    }

    template<int WIDE>
    static inline int write_bin(char* dst, s32 dst_len, unsigned long long number)
    {
        static const char* lut =
            "0123456789abcdefghijk";

        int real_wide = 0;
#ifndef WIN32
        real_wide = sizeof(number) * 8 - __builtin_clzll(number);
#else
        unsigned long win_index = 0;
        _BitScanReverse64(&win_index, number);
        real_wide = (int)win_index + 1;
#endif 
        if (real_wide < WIDE)
        {
            real_wide = WIDE;
        }
        if (dst_len < real_wide)
        {
            return 0;
        }
        unsigned long long cur_wide = real_wide;
        do
        {
            const unsigned long long m2 = number & 1;
            number >>= 1;
            *(dst + cur_wide - 1) = lut[m2];
            cur_wide--;
        } while (number);

        *(dst + real_wide) = '\0';
        return real_wide;
    }

    static inline int write_double(char* dst, s32 dst_len, double number)
    {
        if (dst_len < 40)
        {
            return 0;
        }
        int fp_class = std::fpclassify(number);
        switch (fp_class)
        {
        case FP_SUBNORMAL:
        case FP_ZERO:
            *dst = '0';
            return 1;
        case FP_INFINITE:
            memcpy(dst, "inf", 3);
            return 3;
        case FP_NAN:
            memcpy(dst, "nan", 3);
            return 3;
        case FP_NORMAL:
            break;
        default:
            return 0;
        }


        double fabst = std::fabs(number);
        if (fabst < 0.0001 || fabst > 0xFFFFFFFFFFFFFFFULL)
        {
            if (fabst < 0.0001 && fabst > 0.0000001)
            {
                sprintf(dst, "%.08lf", fabst);
            }
            else
            {
                char* buf = gcvt(number, 16, dst);
                (void)buf;
            }
            return (int)strlen(dst);
        }
        bool is_neg = std::signbit(number);
        int neg_offset = 0;
        if (is_neg)
        {
            *dst = '-';
            neg_offset = 1;
        }

        double intpart = 0;
        unsigned long long fractpart = (unsigned long long)(modf(fabst, &intpart) * 10000);
        int base_offset = write_ull<0>(dst + neg_offset, dst_len - neg_offset, (unsigned long long)intpart);
        if (fractpart > 0)
        {
            *(dst + neg_offset + base_offset) = '.';
            int fractpat_offset = 1 + write_ull<4>(dst + neg_offset + base_offset + 1, dst_len- (neg_offset + base_offset + 1), (unsigned long long)fractpart);
            for (int i = neg_offset + base_offset + fractpat_offset - 1; i > neg_offset + base_offset + 2; i--)
            {
                if (*(dst + i) == '0')
                {
                    fractpat_offset--;
                    continue;
                }
                break;
            }
            return neg_offset + base_offset + fractpat_offset;
        }
        return neg_offset + base_offset;
    }


    inline int write_date(char* dst, s32 len, long long timestamp, unsigned int precise)
    {
        static thread_local tm cache_date = { 0 };
        static thread_local long long cache_timestamp = 0;
        static const char date_fmt[] = "[20190412 13:05:35.417]";
        if (len < sizeof(date_fmt))
        {
            return 0;
        }

        long long day_second = timestamp - cache_timestamp;
        if (day_second < 0 || day_second >= 24 * 60 * 60)
        {
            if (true)
            {
                time_t tv = (time_t)timestamp;
#ifdef WIN32
#if _MSC_VER < 1400 //VS2003
                cache_date = *localtime(&tv);
#else //vs2005->vs2013->
                localtime_s(&cache_date, &tv);
#endif
#else //linux
                localtime_r(&tv, &cache_date);
#endif
            }
            struct tm daytm = cache_date;
            daytm.tm_hour = 0;
            daytm.tm_min = 0;
            daytm.tm_sec = 0;
            cache_timestamp = mktime(&daytm);
            day_second = timestamp - cache_timestamp;
        }
        int write_bytes = 0;

        *(dst + write_bytes++) = '[';

        write_bytes += write_ull<4>(dst + write_bytes, len - write_bytes, (unsigned long long)cache_date.tm_year + 1900);
        write_bytes += write_ull<2>(dst + write_bytes, len - write_bytes, (unsigned long long)cache_date.tm_mon + 1);
        write_bytes += write_ull<2>(dst + write_bytes, len - write_bytes, (unsigned long long)cache_date.tm_mday);

        *(dst + write_bytes++) = ' ';

        write_bytes += write_ull<2>(dst + write_bytes, len - write_bytes, (unsigned long long)day_second / 3600);
        *(dst + write_bytes++) = ':';
        day_second %= 3600;
        write_bytes += write_ull<2>(dst + write_bytes, len - write_bytes, (unsigned long long)day_second / 60);
        *(dst + write_bytes++) = ':';
        day_second %= 60;
        write_bytes += write_ull<2>(dst + write_bytes, len - write_bytes, (unsigned long long)day_second);

        *(dst + write_bytes++) = '.';
        if (precise >= 1000)
        {
            precise = 999;
        }
        write_bytes += write_ull<3>(dst + write_bytes, len - write_bytes, (unsigned long long)precise);

        *(dst + write_bytes++) = ']';

        if (write_bytes != sizeof(date_fmt) - 1)
        {
            return 0;
        }

        return write_bytes;
    }

}

class zstream
{
public:
    char* buf_;
    s32 buf_len_;
    s32 offset_;
public:
    zstream()
    {
        buf_ = nullptr;
        buf_len_ = 0;
        offset_ = 0;
    }
    zstream(char* buf, s32 len, s32 offset = 0)
    {
        attach(buf, len, offset);
    }

    s32 attach(char* buf, s32 len, s32 offset = 0)
    {
        buf_ = buf;
        buf_len_ = len;
        offset_ = offset;
        if (offset < len && len > 0)
        {
            *(buf + offset) = '\0';
        }
        return 0;
    }

    //close with '\0'  
    zstream& write_block(const char* bin, s32 len)
    {
        if (bin == nullptr)
        {
            return *this;
        }
        if (offset_ + len + 1 < buf_len_)
        {
            memcpy(buf_, bin, len);
            offset_ += len;
            *(buf_ + offset_) = '\0';
        }
        return *this;
    }

    zstream& write_str(const char* str, s32 len = 0)
    {
        if (str == nullptr)
        {
            return *this;
        }
        if (len == 0)
        {
            len = (s32)strlen(str);
        }

        if (offset_ + len + 1 < buf_len_)
        {
            memcpy(buf_, str, len + 1);
            offset_ += len;
            *(buf_ + offset_) = '\0';
        }
        return *this;
    }

    zstream& write_s8(char ch)
    {
        if (offset_ + 1 < buf_len_)
        {
            *(buf_ + offset_++) = ch;
            *(buf_ + offset_) = '\0';
        }
        return *this;
    }
    zstream& write_u8(unsigned char ch)
    {
        offset_ += zstream_impl::write_ull<0>(buf_ + offset_, buf_len_ - offset_, ch);
        return *this;
    }

    template<s32 Wide>
    zstream& write_s16(s16 val)
    {
        offset_ += zstream_impl::write_ll<Wide>(buf_ + offset_, buf_len_ - offset_, val);
        return *this;
    }

    template<s32 Wide>
    zstream& write_u16(u16 val)
    {
        offset_ += zstream_impl::write_ull<Wide>(buf_ + offset_, buf_len_ - offset_, val);
        return *this;
    }

    template<s32 Wide>
    zstream& write_s32(s32 val)
    {
        offset_ += zstream_impl::write_ll<Wide>(buf_ + offset_, buf_len_ - offset_, val);
        return *this;
    }

    template<s32 Wide>
    zstream& write_u32(u32 val)
    {
        offset_ += zstream_impl::write_ull<Wide>(buf_ + offset_, buf_len_ - offset_, val);
        return *this;
    }

    template<s32 Wide>
    zstream& write_s64(s64 val)
    {
        offset_ += zstream_impl::write_ll<Wide>(buf_ + offset_, buf_len_ - offset_, val);
        return *this;
    }

    template<s32 Wide>
    zstream& write_u64(u64 val)
    {
        offset_ += zstream_impl::write_ull<Wide>(buf_ + offset_, buf_len_ - offset_, val);
        return *this;
    }

    zstream& write_float(float val)
    {
        offset_ += zstream_impl::write_double(buf_ + offset_, buf_len_ - offset_, (double)val);
        return *this;
    }

    zstream& write_double(double val)
    {
        offset_ += zstream_impl::write_double(buf_ + offset_, buf_len_ - offset_, (double)val);
        return *this;
    }

    template<s32 Wide>
    zstream& write_hex(u64 val)
    {
        offset_ += zstream_impl::write_hex<Wide>(buf_ + offset_, buf_len_ - offset_, val);
        return *this;
    }

    template<s32 Wide>
    zstream& write_ptr(const void* ptr)
    {
        write_str("0x", 2);
        offset_ += zstream_impl::write_hex<Wide>(buf_ + offset_, buf_len_ - offset_, (u64)ptr);
        return *this;
    }

//stream
    zstream& operator <<(const char* str) { return write_str(str); }
    zstream& operator <<(const void* ptr) { return write_ptr<0>(ptr); }
    zstream& operator <<(char val) { return write_s8(val); }
    zstream& operator <<(u8 val) { return write_u8(val); }
    zstream& operator <<(s16 val) { return write_s16<0>(val); }
    zstream& operator <<(u16 val) { return write_u16<0>(val); }
    zstream& operator <<(s32 val) { return write_s32<0>(val); }
    zstream& operator <<(u32 val) { return write_u32<0>(val); }
    zstream& operator <<(s64 val) { return write_s64<0>(val); }
    zstream& operator <<(u64 val) { return write_u64<0>(val); }
    zstream& operator <<(float val) { return write_float(val); }
    zstream& operator <<(double val) { return write_double(val); }


    zstream& operator <<(const std::string& str) { return write_str(str.c_str(), (s32)str.length()); }
};


#endif