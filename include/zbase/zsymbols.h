
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



//limit symbol count 64k 
//limit symbol len  64k 
class zsymbols
{
public:
    using symbol_head = u32;

    constexpr static u32 head_size = sizeof(symbol_head);
    constexpr static u32 invalid_symbols_id = head_size;
    constexpr static u32 invalid_symbols_section_size = head_size + sizeof("invalid symbol");
    constexpr static u32 empty_symbols_id = invalid_symbols_section_size + head_size;
    constexpr static u32 empty_symbols_secion_size = head_size + sizeof("");

    constexpr static u32 first_exploit_offset = invalid_symbols_section_size + empty_symbols_secion_size; //end head size is (0)  

public:
    char* table_space_;
    s32 table_space_len_;
    s32 exploit_offset_;

    s32 attach(char* table_space, s32 table_space_len, s32 exploit_offset = 0)
    {
        if (table_space == nullptr)
        {
            return -1;
        }
        if (table_space_len < first_exploit_offset + head_size)
        {
            return -2;
        }
        if (exploit_offset != 0 && exploit_offset < first_exploit_offset)
        {
            return -3;
        }

        table_space_ = table_space;
        table_space_len_ = table_space_len;
        exploit_offset_ = exploit_offset;

        if (exploit_offset == 0)
        {

            symbol_head next_offset = invalid_symbols_section_size + head_size;
            memcpy(table_space_, &next_offset, sizeof(next_offset));
            memcpy(table_space_ + sizeof(next_offset), "invalid symbol", sizeof("invalid symbol"));
            exploit_offset_ += sizeof(next_offset) + sizeof("invalid symbol");

            next_offset = exploit_offset_ + empty_symbols_secion_size + head_size;
            memcpy(table_space_ + exploit_offset_, &next_offset, sizeof(next_offset));
            memcpy(table_space_ + exploit_offset_ + sizeof(next_offset), "", sizeof(""));
            exploit_offset_ += sizeof(next_offset) + sizeof("");


            next_offset = 0;
            memcpy(table_space_ + exploit_offset_, &next_offset, sizeof(next_offset));

        }

        return 0;
    }

    const char* symbol_name(u32 name_id)
    {
        if (name_id < table_space_len_)
        {
            return &table_space_[name_id];
        }
        return "";
    }

    u32 add_symbol(const char* name, s32 name_len, bool reuse_same_name)
    {
        if (name == nullptr)
        {
            name = "";
            name_len = 0;
        }
        if (name_len == 0)
        {
            name_len = strlen(name);
        }
        if (exploit_offset_ < first_exploit_offset)
        {
            return invalid_symbols_id;
        }
        if (table_space_ == nullptr)
        {
            return invalid_symbols_id;
        }


        if (reuse_same_name)
        {
            symbol_head next_offset = invalid_symbols_id;
            do
            {
                next_offset = *(symbol_head*)&table_space_[next_offset - head_size];
                if (next_offset == 0)
                {
                    break;
                }
                if (strcmp(&table_space_[next_offset], name) == 0 )
                {
                    return next_offset;
                }

            } while (true);
        }


        s32 new_symbol_len = head_size + name_len + 1;

        if (exploit_offset_ + new_symbol_len + head_size > table_space_len_)
        {
            return invalid_symbols_id;
        }

        symbol_head next_offset = exploit_offset_ + new_symbol_len + head_size;
        memcpy(table_space_ + exploit_offset_, &next_offset, sizeof(next_offset));
        memcpy(table_space_ + exploit_offset_ + sizeof(next_offset), name, name_len + 1);

        next_offset = 0;
        memcpy(table_space_ + exploit_offset_, &next_offset, sizeof(next_offset));
        return 0;
    }

};





#endif