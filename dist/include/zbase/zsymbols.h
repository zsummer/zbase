
/*
* zbase License
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





#ifndef  ZSYMBOLS_H
#define ZSYMBOLS_H

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


class zsymbols
{
public:
    using symbol_head = u32;

    constexpr static s32 head_size = sizeof(symbol_head);
    constexpr static s32 invalid_symbols_id = head_size;
    constexpr static s32 invalid_symbols_section_size = head_size + sizeof("invalid symbol");
    constexpr static s32 empty_symbols_id = invalid_symbols_section_size + head_size;
    constexpr static s32 empty_symbols_secion_size = head_size + sizeof("");

    constexpr static s32 first_exploit_offset = invalid_symbols_section_size + empty_symbols_secion_size;
    constexpr static s32 min_space_size = first_exploit_offset + head_size;  

public:
    char* space_;
    s32 space_len_;
    s32 exploit_;

    s32 attach(char* space, s32 space_len, s32 exploit_offset = 0)
    {
        if (space == nullptr)
        {
            return -1;
        }
        if (space_len < min_space_size)
        {
            return -2;
        }
        if (exploit_offset != 0 && exploit_offset < first_exploit_offset)
        {
            return -3;
        }

        space_ = space;
        space_len_ = space_len;
        exploit_ = exploit_offset;

        if (exploit_offset == 0)
        {

            symbol_head next_offset = invalid_symbols_section_size + head_size;
            memcpy(space_, &next_offset, sizeof(next_offset));
            memcpy(space_ + sizeof(next_offset), "invalid symbol", sizeof("invalid symbol"));
            exploit_ += sizeof(next_offset) + sizeof("invalid symbol");

            next_offset = exploit_ + empty_symbols_secion_size + head_size;
            memcpy(space_ + exploit_, &next_offset, sizeof(next_offset));
            memcpy(space_ + exploit_ + sizeof(next_offset), "", sizeof(""));
            exploit_ += sizeof(next_offset) + sizeof("");


            next_offset = 0;
            memcpy(space_ + exploit_, &next_offset, sizeof(next_offset));

        }

        return 0;
    }

    const char* symbol_name(s32 name_id) const
    {
        if (name_id < space_len_)
        {
            return &space_[name_id];
        }
        return "";
    }

    const char* at(s32 name_id) const { return symbol_name(name_id); }
    const char* name(s32 name_id) const { return symbol_name(name_id); }
    s32 add(const char* name, s32 name_len, bool reuse_same_name) { return add_symbol(name, name_len, reuse_same_name); }

    s32 add_symbol(const char* name, s32 name_len, bool reuse_same_name)
    {
        if (name == nullptr)
        {
            name = "";
            name_len = 0;
        }
        if (name_len == 0)
        {
            name_len = (s32)strlen(name);
        }
        if (exploit_ < first_exploit_offset)
        {
            return invalid_symbols_id;
        }
        if (space_len_ < min_space_size)
        {
            return invalid_symbols_id;
        }
        if (space_ == nullptr)
        {
            return invalid_symbols_id;
        }


        if (reuse_same_name)
        {
            symbol_head next_offset = invalid_symbols_id;
            do
            {
                next_offset = *(symbol_head*)&space_[next_offset - head_size];
                if (next_offset == 0)
                {
                    break;
                }
                if (strcmp(&space_[next_offset], name) == 0 )
                {
                    return next_offset;
                }

            } while (true);
        }


        s32 new_symbol_len = head_size + name_len + 1;

        if (exploit_ + new_symbol_len + head_size > space_len_)
        {
            return invalid_symbols_id;
        }

        s32 id = exploit_ + head_size;
        symbol_head next_offset = exploit_ + new_symbol_len + head_size;
        memcpy(space_ + exploit_, &next_offset, sizeof(next_offset));
        memcpy(space_ + exploit_ + sizeof(next_offset), name, name_len + 1);
        exploit_ += new_symbol_len;
        next_offset = 0;
        memcpy(space_ + exploit_, &next_offset, sizeof(next_offset));
        return id;
    }

    s32 clone_from(const zsymbols& from)
    {
        if (space_ == nullptr)
        {
            return -1;
        }
        if (space_ == from.space_)
        {
            return -2;
        }

        if (from.space_ == nullptr || from.space_len_ < min_space_size)
        {
            return -3;
        }

        //support shrink  
        if (space_len_ < from.exploit_ + head_size)
        {
            return -3;
        }
        memcpy(space_, from.space_, (s64)from.exploit_ + head_size);
        exploit_ = from.exploit_;
        return 0;
    }
};

template<s32 TableLen>
class zsymbols_static :public zsymbols
{
public:
    zsymbols_static()
    {
        static_assert(TableLen > zsymbols::first_exploit_offset, "");
        attach(space_, TableLen);
    }
private:
    char space_[TableLen];
};


#endif