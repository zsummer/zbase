
/*
* zshm_loader License
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

#pragma once
#ifndef _ZSHM_LOADER_H_
#define _ZSHM_LOADER_H_

#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <array>
#include <mutex>
#include <thread>
#include <functional>
#include <regex>
#include <atomic>
#include <cmath>
#include <cfloat>
#include <list>
#include <deque>
#include <queue>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <memory>
#include <atomic>
#include <cstddef>

#ifdef WIN32

#ifndef KEEP_INPUT_QUICK_EDIT
#define KEEP_INPUT_QUICK_EDIT false
#endif

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <io.h>
#include <shlwapi.h>
#include <process.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "User32.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4996)

#else
#include <sys/mman.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif


#ifdef __APPLE__
#include "TargetConditionals.h"
#include <dispatch/dispatch.h>
#if !TARGET_OS_IPHONE
#define NFLOG_HAVE_LIBPROC
#include <libproc.h>
#endif
#endif


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





#ifdef WIN32

class zshm_loader_win32
{
public:
	zshm_loader_win32()
	{
		reset();
	}

	void reset()
	{
		map_file_ = nullptr;
		shm_key_ = 0;
		shm_mem_size_ = 0;
		shm_mnt_addr_ = nullptr;
	}
	s64 shm_mem_size() { return shm_mem_size_; }
	void* shm_mnt_addr() { return shm_mnt_addr_; }
private:
	HANDLE map_file_;
	u64 shm_key_;
	s64 shm_mem_size_;
	void* shm_mnt_addr_;
public:
	bool check_exist(u64 shm_key, s64 mem_size)
	{
		if (shm_key <= 0)
		{
			return false;
		}

		shm_key_ = shm_key;
		shm_mem_size_ = mem_size;


		std::string str_key = std::to_string(shm_key);
		HANDLE handle = OpenFileMapping(PAGE_READWRITE, FALSE, str_key.c_str());

		if (handle == nullptr)
		{
			return false;
		}

		map_file_ = handle;
		return true;
	}


	s32 load_from_shm(u64 expect_addr = 0)
	{
		if (map_file_ == nullptr)
		{
			//no shm  
			return -1;
		}


		std::string str_key = std::to_string(shm_key_);
		HANDLE handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, shm_mem_size_ >> 32, shm_mem_size_ & (MAXUINT32), str_key.c_str());
		if (handle == nullptr)
		{
			return -2;
		}
		CloseHandle(map_file_);
		map_file_ = handle;


		LPVOID addr = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, shm_mem_size_);
		if (addr == nullptr)
		{
			volatile DWORD dw = GetLastError();
			(void)dw;
			return -2;
		}
		shm_mnt_addr_ = addr;

		// 资源安全: view和handle双引用  
		// 共享策略: 至少还有一个进程的handle的存活 && 单个进程不能多个view  
		//CloseHandle(map_file_);
		//map_file_ = nullptr;
		return 0;
	}

	s32 create_from_shm(u64 expect_addr = 0)
	{
		if (shm_key_ == 0)
		{
			//no shm key   
			return -1;
		}
		std::string str_key = std::to_string(shm_key_);
		//可读写共享  
		//创建且不允许已经存在  
		HANDLE handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, shm_mem_size_ >> 32, shm_mem_size_ & (MAXUINT32), str_key.c_str());
		if (handle == nullptr)
		{
			return -2;
		}
		map_file_ = handle;

		LPVOID addr = MapViewOfFile(map_file_, FILE_MAP_ALL_ACCESS, 0, 0, shm_mem_size_);
		if (addr == nullptr)
		{
			return -2;
		}
		shm_mnt_addr_ = addr;
		//*(u64*)shm_mnt_addr_ = 1;

		// 资源安全: view和handle双引用  
		// 共享策略: 至少还有一个进程的handle的存活 && 单个进程不能多个view  
		//CloseHandle(map_file_);
		//map_file_ = nullptr;
		return 0;
	}

	bool is_attach()
	{
		return shm_mnt_addr_ != nullptr;
	}


	//
	s32 detach()
	{
		if (is_attach())
		{
			//所有handle都unmap后会数据落地  
			//这之前可以FlushViewOfFile   
			UnmapViewOfFile(shm_mnt_addr_);
			shm_mnt_addr_ = nullptr;
		}
		return 0;
	}

	s32 destroy()
	{
		detach();
		//win没有destroy mapping接口 而是所有handle被close   
		if (map_file_ != nullptr)
		{
			CloseHandle(map_file_);
			map_file_ = nullptr;
		}
		return 0;
	}

	
	static s32 static_destroy(u64 shm_key, void* real_addr, s64 mem_size)
	{
		std::string str_key = std::to_string(shm_key);
		HANDLE handle = OpenFileMapping(PAGE_READWRITE, FALSE, str_key.c_str());
		if (handle == nullptr)
		{
			return -1;
		}
		if (real_addr != nullptr)
		{
			UnmapViewOfFile(real_addr);
			real_addr = nullptr;
		}
		CloseHandle(handle);
		return 0;
	}
};


#else

class zshm_loader_unix
{
public:
	zshm_loader_unix()
	{
		reset();
	}
	void reset()
	{
		shm_key_ = 0;
		shm_index_ = -1;
		shm_mem_size_ = 0;
		shm_mnt_addr_ = nullptr;
	}
	s64 shm_mem_size() { return shm_mem_size_; }
	void* shm_mnt_addr() { return shm_mnt_addr_; }
private:
	u64 shm_key_;
	s32 shm_index_;
	s64 shm_mem_size_;
	void* shm_mnt_addr_;
public:
	bool check_exist(u64 shm_key, s64 mem_size)
	{
		if (shm_key <= 0)
		{
			return false;
		}
		shm_key_ = shm_key;
		shm_mem_size_ = mem_size;
		int idx = shmget(shm_key, 0, 0);
		if (idx < 0 && errno != ENOENT)
		{
			return false;
		}
		if (idx < 0)
		{
			return false;
		}
		
		shm_index_ = idx;
		
		return true;
	}


	s32 load_from_shm(u64 expect_addr = 0)
	{
		if (shm_index_ == -1)
		{
			//no shm  
			return -1;
		}
		void* addr = shmat(shm_index_, (void*)expect_addr, 0);
		if (addr == nullptr || addr == (void*)-1)
		{
			//attach error. 
			return -2;
		}
		shm_mnt_addr_ = addr;
		return 0;
	}

	s32 create_from_shm(u64 expect_addr = 0)
	{
		if (shm_key_ == 0)
		{
			//no shm key   
			return -1;
		}
		//可读写共享  
		//创建且不允许已经存在  
		s32 idx = shmget(shm_key_, shm_mem_size_, IPC_CREAT | IPC_EXCL | 0600);
		if (idx < 0)
		{
			return -2;
		}
		shm_index_ = idx;

		void* addr = shmat(shm_index_, (void*)expect_addr, 0);
		if (addr == nullptr || addr == (void*)-1)
		{
			//attach error. 
			return -3;
		}

		shm_mnt_addr_ = addr;
		return 0;
	}
	bool is_attach()
	{
		return shm_mnt_addr_ != nullptr;
	}

	s32 detach()
	{
		if (is_attach())
		{
			shmdt(shm_mnt_addr_);
			shm_mnt_addr_ = nullptr;
		}

		return 0;
	}

	s32 destroy()
	{
		detach();

		if (shm_index_ >= 0)
		{
			shmctl(shm_index_, IPC_RMID, nullptr);
			shm_index_ = -1;
		}
		return 0;
	}

	static s32 static_destroy(u64 shm_key, void* real_addr, s64 mem_size)
	{
		int idx = shmget(shm_key, 0, 0);
		if (idx < 0 && errno != ENOENT)
		{
			return -1;
		}
		if (idx < 0)
		{
			return -2;
		}
		if (real_addr != nullptr)
		{
			shmdt(real_addr);
		}
		shmctl(idx, IPC_RMID, nullptr);
		return 0;
	}
};

#endif // WIN32




class zshm_loader_heap
{
public:
	zshm_loader_heap()
	{
		reset();
	}
	void reset()
	{
		shm_mnt_addr_ = nullptr;
		shm_mem_size_ = 0;
	}
	s64 shm_mem_size() { return shm_mem_size_; }
	void* shm_mnt_addr() { return shm_mnt_addr_; }
private:
	void* shm_mnt_addr_;
	s64 shm_mem_size_;
public:
	bool check_exist(u64 shm_key, s64 mem_size)
	{
		shm_mem_size_ = mem_size;
		return false;
	}


	s32 load_from_shm(u64 expect_addr = 0)
	{
		return -1;
	}

	s32 create_from_shm(u64 expect_addr = 0)
	{
#ifdef WIN32
		//提交内存不代表实际使用 但windows下系统提交内存总大小不能超过物理内存+交换文件 否则会内存不足.  
		char* addr = (char*)VirtualAlloc((LPVOID)expect_addr, shm_mem_size_, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
#else
		char* addr = (char*)mmap(NULL, shm_mem_size_, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#endif // WIN32
		if (addr == nullptr)
		{
			return -1;
		}
		shm_mnt_addr_ = addr;
		return 0;
	}


	bool is_attach()
	{
		return shm_mnt_addr_ != nullptr;
	}

	s32 detach()
	{
		if (is_attach())
		{
#ifdef WIN32
			VirtualFree(shm_mnt_addr_, 0, MEM_RELEASE);
#else
			munmap(shm_mnt_addr_, shm_mem_size_);
#endif // WIN32


			shm_mnt_addr_ = nullptr;
		}

		return 0;
	}

	s32 destroy()
	{
		detach();
		return 0;
	}


	static s32 static_destroy(u64 shm_key, void* real_addr, s64 mem_size)
	{
		if (real_addr == nullptr)
		{
			return -1;
		}
#ifdef WIN32
		VirtualFree(real_addr, 0, MEM_RELEASE);
#else
		munmap(real_addr, mem_size);
#endif // WIN32
		return 0;
	}
};




class zshm_loader
{
#ifdef WIN32
	zshm_loader_win32 loader_;
#else
	zshm_loader_unix loader_;
#endif // WIN32
	zshm_loader_heap heap_loader_;
	bool used_heap_;
public:
	zshm_loader()
	{
		used_heap_ = false;
	}
	zshm_loader(bool used_heap)
	{
		used_heap_ = used_heap;
	}

	~zshm_loader()
	{
	}

	s64 shm_mem_size() { return used_heap_ ? heap_loader_.shm_mem_size() : loader_.shm_mem_size(); }
	void* shm_mnt_addr() { return used_heap_ ? heap_loader_.shm_mnt_addr() : loader_.shm_mnt_addr(); }

	bool check_exist(u64 shm_key, s64 mem_size){return used_heap_ ? heap_loader_.check_exist(shm_key, mem_size): loader_.check_exist(shm_key, mem_size);}

	s32 load_from_shm(u64 expect_addr = 0){return used_heap_ ? heap_loader_.load_from_shm(expect_addr): loader_.load_from_shm(expect_addr);}
	s32 create_from_shm(u64 expect_addr = 0){return used_heap_ ? heap_loader_.create_from_shm(expect_addr): loader_.create_from_shm(expect_addr);}

	bool is_attach(){return used_heap_ ? heap_loader_.is_attach(): loader_.is_attach();}

	s32 detach(){return used_heap_ ? heap_loader_.detach(): loader_.detach();}

	s32 destroy(){return used_heap_ ? heap_loader_.destroy(): loader_.destroy();}

	void reset(){ used_heap_ ? heap_loader_.reset(): loader_.reset();}

	static s32 static_destroy(u64 shm_key, s32 use_heap, void* real_addr, s64 mem_size) 
	{ 
		if (use_heap)
		{
			return zshm_loader_heap::static_destroy(shm_key, real_addr, mem_size);
		}
#ifdef WIN32
		return zshm_loader_win32::static_destroy(shm_key, real_addr, mem_size);
#else
		return zshm_loader_unix::static_destroy(shm_key, real_addr, mem_size);
#endif // WIN32
	}
};




#endif



/*

mmap最小起始地址略小于0x0000 7F00 0000 0000

安全中心: 0x0000 5655 5555 5555 + (0x0000 7F00 0000 0000 - 0x0000 5655 5555 5555)/2   =  0x0000 6AAA AAAA AAAA

brk起始地址最大略大于 0x0000 5655 5555 5555

*/

