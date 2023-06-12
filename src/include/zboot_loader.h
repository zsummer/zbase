/*
* zboot_loader License
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


#ifndef  _ZBOOT_LOADER_H_
#define _ZBOOT_LOADER_H_


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


#include "zshm_loader.h"
#include "zarray.h"

#define ZSHM_MAX_SPACE_SEGMENTS 20


//desc  
struct zshm_addres
{
	u64 version_;
	u64 hash_;
	u64 offset_;
	u64 size_;
};

//desc  
struct zshm_space_entry
{
	u64 space_addr_;
	u64 shm_key_;
	s32 use_heap_;
	s32 used_fixed_address_;
	zshm_addres whole_space_;
	std::array<zshm_addres, ZSHM_MAX_SPACE_SEGMENTS> segments_;
};


//用完即丢 
class zboot_loader
{
public:
	zboot_loader() {}
	~zboot_loader() {}

	s32 build_frame(const zshm_space_entry& params, zshm_space_entry*& entry)
	{
		entry = nullptr;
		zshm_loader loader(params.use_heap_);
		bool is_exist = loader.check_exist(params.shm_key_, params.whole_space_.size_);
		if (is_exist)
		{
			return -1;
		}
		s32 ret = loader.create_from_shm(params.space_addr_);
		if (ret != 0)
		{
			return -2;
		}
		memcpy(loader.shm_mnt_addr(), &params, sizeof(params));
		entry = static_cast<zshm_space_entry*>(loader.shm_mnt_addr());
		entry->space_addr_ = (u64)(loader.shm_mnt_addr());
		if (params.used_fixed_address_)
		{
			loader.destroy();
			entry = nullptr;
			return -3;
		}
		return 0;
	}

	s32 resume_frame(const zshm_space_entry& params, zshm_space_entry*& entry)
	{
		entry = nullptr;
		zshm_loader loader(params.use_heap_);
		bool is_exist = loader.check_exist(params.shm_key_, params.whole_space_.size_);
		if (!is_exist)
		{
			return -1;
		}
		s32 ret = loader.load_from_shm(params.space_addr_);
		if (ret != 0)
		{
			return -2;
		}
		entry = static_cast<zshm_space_entry*>(loader.shm_mnt_addr());
		entry->space_addr_ = (u64)(loader.shm_mnt_addr());
		if (false)
		{
			//resume失败要手动清理shm  防止误删shm数据集  
			//loader.destroy();
			entry = nullptr;
			return -3;
		}

		//check version  
		if (memcmp(&entry->whole_space_, &params.whole_space_, sizeof(params.whole_space_)) != 0)
		{
			entry = nullptr;
			return -4;
		}
		if (memcmp(&entry->segments_, &params.segments_, sizeof(params.segments_)) != 0)
		{
			entry = nullptr;
			return -5;
		}

		return 0;
	}

private:

};



#endif