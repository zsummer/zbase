
/*
* zbitset License
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
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





#ifndef  ZBITSET_H
#define ZBITSET_H

#include <type_traits>
#include <iterator>
#include <cstddef>
#include <memory>
#include <algorithm>
#include <bitset>

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


//如果不知道该使用zbitset还是std::bitset应该使用后者  
//zbitset提供的是在一段连续平台内存上提供bitset的操作方法 相当于bitset的wrapper   

class zbitset
{
public:
    static constexpr u32 kByteSize = sizeof(u64);
    static constexpr u32 kBitWide = kByteSize * 8;
    static constexpr u32 kBitWideMask = kBitWide - 1;


    static constexpr u64 kBaseMask = (u64)0 - (u64)1;

    static constexpr u32 ceil_array_size(u32 bit_count) { return (bit_count + kBitWideMask) / kBitWide; }
    static constexpr u32 max_bit_count(u32 array_size) { return kBitWide * array_size; }


public:
    static_assert(std::is_unsigned<u64>::value, "only support unsigned long long");
    static_assert(sizeof(u64) == 8, "only support unsigned long long");


private:
    u64* array_data_;
    u32  array_size_;
    u32  bit_count_;
    u32 has_error_;
    u32 dirty_count_;
    u32 win_min_;
    u32 win_max_;

public:
    u64* array_data() const { return array_data_; }
    u32 array_size() const { return array_size_; }
    u32 bit_count() const { return bit_count_; }
    u32 has_error() const { return has_error_; }
    u32 first_bit() const { return win_min_ < bit_count_ ? win_min_ * kBitWide : 0; }
    u32 win_size() const { return win_max_ > win_min_ ? win_max_ - win_min_ : 0; }
    u32 dirty_count() const { return dirty_count_; }
    bool empty() const { return dirty_count_ == 0; }
private:
    u32 win_min() const { return win_min_; }
    u32 win_max() const { return win_max_; }
public:
    zbitset()
    {
        array_data_ = nullptr;
        array_size_ = 0;
        bit_count_ = 0;
        has_error_ = 0;
        win_min_ = 0;
        win_max_ = 0;
    }
    ~zbitset()
    { 
    }

    // mem的地址要求8字节对齐  否则会有性能和兼容性问题, 非u64数组的定义可以用 alignas(sizeof(u64))  
    void attach(u64* u64_array, u32 array_size, bool with_clean)
    {
        array_data_ = u64_array;
        array_size_ = array_size;
        bit_count_ = max_bit_count(array_size);
        light_clear();
        if (with_clean)
        {
            clear();
        }
    }

    s32 clone_from(const zbitset& bmap)
    {
        if (array_size_ != bmap.array_size())
        {
            return 1;
        }
        if (bit_count_ != bmap.bit_count())
        {
            return 2;
        }

        if (array_size_ == 0)
        {
            return 3;
        }

        memcpy(array_data_, bmap.array_data(), bmap.array_size() *sizeof(u64));
        has_error_ = bmap.has_error();
        win_min_ = bmap.win_min();
        win_max_ = bmap.win_max();
        return 0;
    }

    void light_clear()
    {
        if (array_size_ == 0)
        {
            return;
        }
        has_error_ = 0;
        win_min_ = bit_count_;
        win_max_ = 0;
        dirty_count_ = 0;
    }

    void clear()
    {
        if (array_size_ == 0)
        {
            return;
        }
        light_clear();
        memset(array_data_, 0, array_size_ * sizeof(u64));
    }
    



    //bit index  
    void set(u32 index)
    {
        if (index >= bit_count_)
        {
            has_error_++;
            return;
        }
        array_data_[index / kBitWide] |= 1ULL << (index % kBitWide) ;
        dirty_count_++;
    }


    void set_with_win(u32 index)
    {
        if (index >= bit_count_)
        {
            has_error_++;
            return;
        }

        set(index);

        u32 array_id = index / kBitWide;
        //less 10ns 
        if (array_id < win_min_)
        {
            win_min_ = array_id;
        }
        if (array_id >= win_max_)
        {
            win_max_ = array_id + 1;
        }
    }

    //bit index  
    void unset(u32 index)
    {
        if (index >= bit_count_)
        {
            return;
        }
        array_data_[index / kBitWide] &= ~(1ULL << (index % kBitWide));
        dirty_count_++;
    }

    void unset_with_win(u32 index)
    {
        if (index >= bit_count_)
        {
            return;
        }

        unset(index);

        u32 array_id = index / kBitWide;
        //less 10ns 
        if (array_id < win_min_)
        {
            win_min_ = array_id;
        }
        if (array_id >= win_max_)
        {
            win_max_ = array_id + 1;
        }
    }


    bool has(u32 index) const
    {
        if (index >= bit_count_)
        {
            return false;
        }
        return array_data_[index / kBitWide] & 1ULL << (index % kBitWide);
    }

 
    u32 pick_next_with_win(u32 bit_id)
    {
        u32 index = bit_id / kBitWide;
        
        while (index < win_max_)
        {
            if (array_data_[index] == 0)
            {
                index++;
                continue;
            }
#ifdef WIN32
            unsigned long bit_index = 0;
            _BitScanForward64(&bit_index, array_data_[index]);
            u32 result_bit_id = index * kBitWide + (u32)bit_index;
#else
            u32 result_bit_id = index * kBitWide + __builtin_ffsll(array_data_[index]) - 1;
#endif // WIN32
            array_data_[index] &= array_data_[index] - 1;
            return result_bit_id;
        }
        return bit_count_;
    }

    u32 pick_next(u32 bit_id)
    {
        u32 index = bit_id / kBitWide;

        while (index < array_size_)
        {
            if (array_data_[index] == 0)
            {
                index++;
                continue;
            }
#ifdef WIN32
            unsigned long bit_index = 0;
            _BitScanForward64(&bit_index, array_data_[index]);
            u32 result_bit_id = index * kBitWide + (u32)bit_index;
#else
            u32 result_bit_id = index * kBitWide + __builtin_ffsll(array_data_[index]) - 1;
#endif // WIN32
            array_data_[index] &= array_data_[index] - 1;
            return result_bit_id;
        }
        return bit_count_;
    }


};

template<u32 _BitCount>
class zbitset_static : public zbitset
{
public:
    static constexpr u32 kArraySize = zbitset::ceil_array_size(_BitCount);
    zbitset_static()
    {
        attach(bitmap_, kArraySize, true);
    }
    zbitset_static(const zbitset_static<_BitCount>& other) :zbitset_static()
    {
        clone_from(other);
    }
private:
    u64 bitmap_[kArraySize];
};




#endif