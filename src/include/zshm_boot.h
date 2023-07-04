
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
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

#define ZSHM_MAX_SPACES 20


//desc  
struct zshm_space
{
	u64 version_;
	u64 hash_;
	u64 offset_;
	u64 size_;
};

//desc  
struct zshm_space_entry
{
	u64 space_;
	u64 shm_key_;
	s32 use_heap_;
	s32 use_fixed_address_;
	zshm_space whole_space_;
	std::array<zshm_space, ZSHM_MAX_SPACES> spaces_;
};




//Òýµ¼   
class zshm_boot
{
public:
	zshm_boot() {}
	~zshm_boot() {}

	static s32 build_frame(const zshm_space_entry& params, zshm_space_entry*& entry)
	{
		entry = nullptr;
		zshm_loader loader(params.use_heap_, params.shm_key_, params.whole_space_.size_);
		s32 ret = loader.check();
		if (ret != zshm_errno::E_NO_SHM_MAPPING)
		{
			return ret;
		}
		ret = loader.create(params.space_);
		if (ret != 0)
		{
			return ret;
		}
		memcpy(loader.shm_mnt_addr(), &params, sizeof(params));
		entry = static_cast<zshm_space_entry*>(loader.shm_mnt_addr());
		entry->space_ = (u64)(loader.shm_mnt_addr());

		return 0;
	}

	static s32 resume_frame(const zshm_space_entry& params, zshm_space_entry*& entry)
	{
		entry = nullptr;
		zshm_loader loader(params.use_heap_, params.shm_key_, params.whole_space_.size_);
		s32 ret = loader.check();
		if (ret != 0)
		{
			return ret;
		}
		ret = loader.attach(params.space_);
		if (ret != 0)
		{
			return ret;
		}

		entry = static_cast<zshm_space_entry*>(loader.shm_mnt_addr());
		entry->space_ = (u64)(loader.shm_mnt_addr());

		//check version  
		if (memcmp(&entry->whole_space_, &params.whole_space_, sizeof(params.whole_space_)) != 0)
		{
			entry = nullptr;
			return zshm_errno::E_SHM_VERSION_MISMATCH;
		}
		if (memcmp(&entry->spaces_, &params.spaces_, sizeof(params.spaces_)) != 0)
		{
			entry = nullptr;
			return zshm_errno::E_SHM_VERSION_MISMATCH;
		}

		return 0;
	}

	static s32 destroy_frame(const zshm_space_entry& params)
	{
		return zshm_loader::external_destroy(params.shm_key_, params.use_heap_, (void*)params.space_, params.whole_space_.size_);
	}

private:

};



#endif