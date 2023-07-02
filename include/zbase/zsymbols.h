
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


//����ʵ�־��ṩ��ͬ�Ķ���������O(1)С��������, ������׷�ӷ�����ʱ�� ���������з�����ʱ����ΪO(M) MΪд��������ĳ���    
//���ģ�ķ�����ӳ��(�����Ϸ�����) Ҫ���⿪��reuse���add����; (Ԥ��¼�������ǿ���reuse) 
//���ַ�������solid����, ����CPU cache �� head���ȵ��ڴ濪�� ���Լ۱�.     
//���ַ�������fast  �ṩ�ַ���������Ϣ    
//���ǳ���ʹ�ó���,  Ĭ��ʹ��solid + reuse����.  

class zsymbols_fast
{
public:
    using symbol_head = s32;
    constexpr static s32 head_size = sizeof(symbol_head);
    constexpr static s32 invalid_symbols_id = 0;
    constexpr static s32 invalid_symbols_section_size = head_size;

    constexpr static s32 first_exploit_offset = invalid_symbols_section_size;
    constexpr static s32 min_space_size = invalid_symbols_section_size;    

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
            symbol_head head = 0;
            memcpy(space_, &head, sizeof(head));
            exploit_ += sizeof(head);
        }

        return 0;
    }

    const char* at(s32 name_id) const 
    {
        if (name_id < space_len_)
        {
            return &space_[name_id];
        }
        return "";
    }

    s32 len(s32 name_id) const
    {
        if (name_id >= first_exploit_offset || name_id < exploit_)
        {
            symbol_head head;
            memcpy(&head, &space_[name_id - sizeof(head)], sizeof(head)); //adapt address align .  
            return head;
        }
        return 0;
    }

    s32 add(const char* name, s32 name_len, bool reuse_same_name) 
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
            s32 offset = first_exploit_offset;
            symbol_head head = 0;

            while (offset + 4 <= exploit_)
            {
                memcpy(&head, space_ + offset, sizeof(head));
                if (offset + 4 + head + 1 > space_len_)
                {
                    break;  //has error  
                }
                if (strcmp(space_ + offset + 4, name) == 0)
                {
                    return offset + 4;
                }
                offset += 4 + head + 1;
            }
        }

        s32 new_symbol_len = head_size + name_len + 1;

        if (exploit_ + new_symbol_len > space_len_)
        {
            return invalid_symbols_id;
        }


        symbol_head head = name_len;
        memcpy(space_ + exploit_, &head, sizeof(head));
        memcpy(space_ + exploit_ + sizeof(head), name, (u64)name_len + 1);
        s32 symbol_id = exploit_ + sizeof(head);
        exploit_ += new_symbol_len;
        return symbol_id;
    }

    s32 clone_from(const zsymbols_fast& from)
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
        if (space_len_ < from.exploit_)
        {
            return -3;
        }
        memcpy(space_, from.space_, (s64)from.exploit_);
        exploit_ = from.exploit_;
        return 0;
    }
};


class zsymbols_solid
{
public:
    constexpr static s32 invalid_symbols_id = 0;
    constexpr static s32 invalid_symbols_section_size = sizeof("invalid symbol");
    constexpr static s32 first_exploit_offset = invalid_symbols_section_size;
    constexpr static s32 min_space_size = first_exploit_offset;

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
            memcpy(space_, "invalid symbol", sizeof("invalid symbol"));
            exploit_ += sizeof("invalid symbol");

            memcpy(space_ + exploit_, "", sizeof(""));
            exploit_ += sizeof("");
        }

        return 0;
    }

    const char* at(s32 name_id) const
    {
        if (name_id < space_len_)
        {
            return &space_[name_id];
        }
        return "";
    }

    
    s32 add(const char* name, s32 name_len, bool reuse_same_name) 
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

        s32 new_symbol_len = name_len + 1;
        if (reuse_same_name)
        {
            s32 find_offset = first_exploit_offset;
            while (find_offset + new_symbol_len <= space_len_)
            {
                if (space_[find_offset + new_symbol_len - 1] == '\0')
                {
                    //todo: optimize
                    if (strcmp(&space_[find_offset], name) == 0)
                    {
                        return find_offset;
                    }
                    find_offset += new_symbol_len;
                    continue;
                }
                find_offset++;
            }
        }


        if (exploit_ + new_symbol_len > space_len_)
        {
            return invalid_symbols_id;
        }

        s32 symbol_id = exploit_;
        memcpy(space_ + exploit_, name, new_symbol_len);
        exploit_ += new_symbol_len;

        return symbol_id;
    }

    s32 clone_from(const zsymbols_solid& from)
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
        if (space_len_ < from.exploit_)
        {
            return -3;
        }
        memcpy(space_, from.space_, (s64)from.exploit_);
        exploit_ = from.exploit_;
        return 0;
    }
};


template<s32 TableLen>
class zsymbols_fast_static :public zsymbols_fast
{
public:
    zsymbols_fast_static()
    {
        static_assert(TableLen >= zsymbols_fast::min_space_size, "");
        attach(space_, TableLen);
    }
private:
    char space_[TableLen];
};

template<s32 TableLen>
class zsymbols_solid_static :public zsymbols_solid
{
public:
    zsymbols_solid_static()
    {
        static_assert(TableLen >= zsymbols_solid::min_space_size, "");
        attach(space_, TableLen);
    }
private:
    char space_[TableLen];
};

using zsymbols = zsymbols_solid;
template<s32 TableLen>
using zsymbols_static = zsymbols_solid_static< TableLen>;

#endif