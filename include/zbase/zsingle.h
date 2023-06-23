
/*
* zvector License
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





#ifndef  ZSINGLE_H
#define ZSINGLE_H

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




template<class T>
class zsingle
{
public:
    zsingle(zsingle const&) = delete;
    zsingle& operator=(zsingle const&) = delete;

    static T& instance()
    {
        static T inst;
        return inst;
    }

protected:
    zsingle() = default;
    ~zsingle() = default;
};



#endif