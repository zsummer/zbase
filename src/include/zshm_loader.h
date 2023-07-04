

/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
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

namespace zshm_errno
{
	enum ZSHM_ERRNO_TYPE :s32
	{
		E_SUCCESS = 0,
		E_NO_INIT,
		E_INVALID_PARAM,
		E_NO_SHM_MAPPING,
		E_INVALID_SHM_MAPPING,
		E_CREATE_SHM_MAPPING_FAILED,
		E_ATTACH_SHM_MAPPING_FAILED,
		E_CREATE_FILE_FAILED,
		E_CREATE_FILE_MAPPING_FAILED,
		E_ATTACH_FILE_MAPPING_FAILED,
		E_SHM_VERSION_MISMATCH,

		E_MAX_ERROR,
	};

#define ZSHM_ERRNO_TO_STRING(code) case code: return #code; 

	static const char* str(s32 error_code)
	{
		switch (error_code)
		{
			ZSHM_ERRNO_TO_STRING(E_SUCCESS);
			ZSHM_ERRNO_TO_STRING(E_NO_INIT);
			ZSHM_ERRNO_TO_STRING(E_INVALID_PARAM);
			ZSHM_ERRNO_TO_STRING(E_NO_SHM_MAPPING);
			ZSHM_ERRNO_TO_STRING(E_INVALID_SHM_MAPPING);
			ZSHM_ERRNO_TO_STRING(E_CREATE_SHM_MAPPING_FAILED);
			ZSHM_ERRNO_TO_STRING(E_ATTACH_SHM_MAPPING_FAILED);
			ZSHM_ERRNO_TO_STRING(E_CREATE_FILE_FAILED);
			ZSHM_ERRNO_TO_STRING(E_CREATE_FILE_MAPPING_FAILED);
			ZSHM_ERRNO_TO_STRING(E_ATTACH_FILE_MAPPING_FAILED);
			ZSHM_ERRNO_TO_STRING(E_SHM_VERSION_MISMATCH);
		}

		return "unknown error";
	}
};



class zshm_loader_win32
{
public:
#ifdef WIN32
	using WIN32_HANDLE = HANDLE;
#else
	using WIN32_HANDLE = void*;
#endif

	zshm_loader_win32()
	{
		init(0, 0);
	}
	void init(u64 shm_key, s64 mem_size)
	{
		shm_key_ = shm_key;
		shm_mem_size_ = mem_size;
		shm_mnt_addr_ = nullptr;
	}

	s64 shm_mem_size() { return shm_mem_size_; }
	void* shm_mnt_addr() { return shm_mnt_addr_; }
private:
	u64 shm_key_;
	s64 shm_mem_size_;
	void* shm_mnt_addr_;
public:
	s32 check()
	{
		if (shm_key_ == 0)
		{
			return zshm_errno::E_NO_INIT;
		}

		std::string str_key = std::to_string(shm_key_);

#ifdef WIN32
		HANDLE handle = ::CreateFile(str_key.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return zshm_errno::E_NO_SHM_MAPPING;
		}


		LARGE_INTEGER file_size;
		if (!GetFileSizeEx(handle, &file_size))
		{
			::CloseHandle(handle);
			return zshm_errno::E_INVALID_SHM_MAPPING;
		}

		if ((s64)file_size.QuadPart != shm_mem_size_)
		{
			::CloseHandle(handle);
			return zshm_errno::E_INVALID_SHM_MAPPING;
		}


		::CloseHandle(handle);
#endif
		return zshm_errno::E_SUCCESS;
	}



	s32 attach(u64 expect_addr = 0)
	{
		if (shm_key_ == 0)
		{
			return zshm_errno::E_NO_INIT;
		}

		std::string str_key = std::to_string(shm_key_);
#ifdef WIN32
		HANDLE handle = ::CreateFile(str_key.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return zshm_errno::E_NO_SHM_MAPPING;
		}


		LARGE_INTEGER file_size;
		if (!GetFileSizeEx(handle, &file_size))
		{
			::CloseHandle(handle);
			return zshm_errno::E_INVALID_SHM_MAPPING;
		}

		if ((s64)file_size.QuadPart != shm_mem_size_)
		{
			::CloseHandle(handle);
			return zshm_errno::E_INVALID_SHM_MAPPING;
		}

		HANDLE mapping_handle = CreateFileMapping(handle, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (mapping_handle == NULL)
		{
			::CloseHandle(handle);
			return zshm_errno::E_ATTACH_FILE_MAPPING_FAILED;
		}

		::CloseHandle(handle);

		LPVOID addr = MapViewOfFileEx(mapping_handle, FILE_MAP_ALL_ACCESS, 0, 0, shm_mem_size_, (void*)expect_addr);
		if (addr == nullptr)
		{
			volatile DWORD dw = GetLastError();
			(void)dw;
			return zshm_errno::E_ATTACH_SHM_MAPPING_FAILED;
		}

		CloseHandle(mapping_handle);
		shm_mnt_addr_ = addr;
#endif
		return 0;
	}


	s32 create(u64 expect_addr = 0)
	{
		if (shm_key_ == 0)
		{
			return zshm_errno::E_NO_INIT;
		}

		std::string str_key = std::to_string(shm_key_);
#ifdef WIN32
		HANDLE handle = ::CreateFile(str_key.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return zshm_errno::E_CREATE_FILE_FAILED;
		}

		LARGE_INTEGER file_size;
		file_size.QuadPart = shm_mem_size_;

		HANDLE mapping_handle = CreateFileMapping(handle, NULL, PAGE_READWRITE, file_size.HighPart, file_size.LowPart, NULL);
		if (mapping_handle == NULL)
		{
			::CloseHandle(handle);
			return zshm_errno::E_CREATE_FILE_MAPPING_FAILED;
		}
		::CloseHandle(handle);


		LPVOID addr = MapViewOfFileEx(mapping_handle, FILE_MAP_ALL_ACCESS, 0, 0, shm_mem_size_, (void*)expect_addr);
		if (addr == nullptr)
		{
			volatile DWORD dw = GetLastError();
			(void)dw;
			return zshm_errno::E_ATTACH_SHM_MAPPING_FAILED;
		}
		CloseHandle(mapping_handle);
		shm_mnt_addr_ = addr;
#endif
		return 0;
	}

	bool is_attach()
	{
		return shm_mnt_addr_ != nullptr;
	}


	//
	s32 detach()
	{
#ifdef WIN32
		if (is_attach())
		{
			//auto flush to file when all views unmaped   
			//FlushViewOfFile   
			UnmapViewOfFile(shm_mnt_addr_);
			shm_mnt_addr_ = nullptr;
		}
#endif
		return 0;
	}

	s32 destroy()
	{
		detach();
		std::string str_key = std::to_string(shm_key_);
		::remove(str_key.c_str());
		return 0;
	}


	static s32 external_destroy(u64 shm_key, void* real_addr, s64 mem_size)
	{
#ifdef WIN32
		if (real_addr != nullptr)
		{
			UnmapViewOfFile(real_addr);
			real_addr = nullptr;
		}
#endif
		std::string str_key = std::to_string(shm_key);
		::remove(str_key.c_str());
		return 0;
	}
};



class zshm_loader_unix
{
public:
	zshm_loader_unix()
	{
		init(0, 0);
	}
	void init(u64 shm_key, s64 mem_size)
	{
		shm_key_ = shm_key;
		shm_index_ = -1;
		shm_mem_size_ = mem_size;
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
	s32 check()
	{
		if (shm_key_ == 0)
		{
			return zshm_errno::E_NO_INIT;
		}
#ifndef WIN32
		int idx = shmget(shm_key_, 0, 0);
		if (idx < 0 && errno == ENOENT)
		{
			return zshm_errno::E_NO_SHM_MAPPING;
		}
		if (idx < 0)
		{
			return zshm_errno::E_INVALID_SHM_MAPPING;
		}
#endif
		return zshm_errno::E_SUCCESS;
	}


	s32 attach(u64 expect_addr = 0)
	{
		if (shm_key_ == 0)
		{
			return zshm_errno::E_NO_INIT;
		}
#ifndef WIN32
		int idx = shmget(shm_key_, 0, 0);
		if (idx < 0 && errno == ENOENT)
		{
			return zshm_errno::E_NO_SHM_MAPPING;
		}
		if (idx < 0)
		{
			return zshm_errno::E_INVALID_SHM_MAPPING;
		}

		void* addr = shmat(idx, (void*)expect_addr, 0);
		if (addr == nullptr || addr == (void*)-1)
		{
			return zshm_errno::E_ATTACH_SHM_MAPPING_FAILED;
		}
		shm_index_ = idx;
		shm_mnt_addr_ = addr;
#endif
		return 0;
	}

	s32 create(u64 expect_addr = 0)
	{
		if (shm_key_ == 0)
		{
			return zshm_errno::E_NO_INIT;
		}
#ifndef WIN32
		//可读写共享  
		//创建且不允许已经存在  
		s32 idx = shmget(shm_key_, shm_mem_size_, IPC_CREAT | IPC_EXCL | 0600);
		if (idx < 0)
		{
			return zshm_errno::E_CREATE_SHM_MAPPING_FAILED;
		}
		

		void* addr = shmat(idx, (void*)expect_addr, 0);
		if (addr == nullptr || addr == (void*)-1)
		{
			return zshm_errno::E_ATTACH_SHM_MAPPING_FAILED;
		}
		shm_index_ = idx;
		shm_mnt_addr_ = addr;
#endif
		return 0;
	}

	bool is_attach()
	{
		return shm_mnt_addr_ != nullptr;
	}

	s32 detach()
	{
#ifndef WIN32
		if (is_attach())
		{
			shmdt(shm_mnt_addr_);
			shm_mnt_addr_ = nullptr;
		}
#endif
		return 0;
	}

	s32 destroy()
	{
		detach();
#ifndef WIN32
		if (shm_index_ >= 0)
		{
			shmctl(shm_index_, IPC_RMID, nullptr);
			shm_index_ = -1;
		}
#endif
		return 0;
	}

	static s32 external_destroy(u64 shm_key, void* real_addr, s64 mem_size)
	{
#ifndef WIN32
		int idx = shmget(shm_key, 0, 0);
		if (idx < 0 && errno == ENOENT)
		{
			return zshm_errno::E_NO_SHM_MAPPING;
		}
		if (idx < 0)
		{
			return zshm_errno::E_INVALID_SHM_MAPPING;
		}
		if (real_addr != nullptr)
		{
			shmdt(real_addr);
		}
		shmctl(idx, IPC_RMID, nullptr);
#endif
		return 0;
	}
};




class zshm_loader_heap
{
public:
	zshm_loader_heap()
	{
		init(0, 0);
	}
	void init(u64 shm_key, s64 mem_size)
	{
		shm_key_ = shm_key;
		shm_mnt_addr_ = nullptr;
		shm_mem_size_ = mem_size;
	}
	s64 shm_mem_size() { return shm_mem_size_; }
	void* shm_mnt_addr() { return shm_mnt_addr_; }
private:
	u64 shm_key_;
	void* shm_mnt_addr_;
	s64 shm_mem_size_;
public:
	s32 check()
	{
		return zshm_errno::E_NO_SHM_MAPPING;
	}

	s32 attach(u64 expect_addr = 0)
	{
		return zshm_errno::E_NO_SHM_MAPPING;
	}

	s32 create(u64 expect_addr = 0)
	{
#ifdef WIN32
		//提交内存不代表实际使用 但windows下系统提交内存总大小不能超过物理内存+交换文件 否则会内存不足.  
		char* addr = (char*)VirtualAlloc((LPVOID)expect_addr, shm_mem_size_, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
#else
		char* addr = (char*)mmap(NULL, shm_mem_size_, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#endif // WIN32
		if (addr == nullptr)
		{
			return zshm_errno::E_CREATE_SHM_MAPPING_FAILED;
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


	static s32 external_destroy(u64 shm_key, void* real_addr, s64 mem_size)
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
	using impl_loader = zshm_loader_win32;
#else
	using impl_loader = zshm_loader_unix;
#endif // WIN32
	impl_loader loader_;
	zshm_loader_heap heap_loader_;
	bool used_heap_;
public:
	zshm_loader(bool used_heap = false, u64 shm_key = 0, s64 mem_size = 0)
	{
		init(used_heap, shm_key, mem_size);
	}

	void init(bool used_heap, u64 shm_key, s64 mem_size)
	{
		used_heap_ = used_heap;
		loader_.init(shm_key, mem_size);
		heap_loader_.init(shm_key, mem_size);
	}

	~zshm_loader()
	{
	}

	s64 shm_mem_size() { return used_heap_ ? heap_loader_.shm_mem_size() : loader_.shm_mem_size(); }
	void* shm_mnt_addr() { return used_heap_ ? heap_loader_.shm_mnt_addr() : loader_.shm_mnt_addr(); }

	s32 check(){return used_heap_ ? heap_loader_.check(): loader_.check();}

	s32 attach(u64 expect_addr = 0){return used_heap_ ? heap_loader_.attach(expect_addr): loader_.attach(expect_addr);}
	s32 create(u64 expect_addr = 0){return used_heap_ ? heap_loader_.create(expect_addr): loader_.create(expect_addr);}

	bool is_attach(){return used_heap_ ? heap_loader_.is_attach(): loader_.is_attach();}

	s32 detach(){return used_heap_ ? heap_loader_.detach(): loader_.detach();}

	s32 destroy(){return used_heap_ ? heap_loader_.destroy(): loader_.destroy();}

	static s32 external_destroy(u64 shm_key, s32 use_heap, void* real_addr, s64 mem_size) 
	{ 
		if (use_heap)
		{
			return zshm_loader_heap::external_destroy(shm_key, real_addr, mem_size);
		}
		return impl_loader::external_destroy(shm_key, real_addr, mem_size);
	}
};


#endif

/*

mmap最小起始地址略小于0x0000 7F00 0000 0000

安全中心: 0x0000 5655 5555 5555 + (0x0000 7F00 0000 0000 - 0x0000 5655 5555 5555)/2   =  0x0000 6AAA AAAA AAAA

brk起始地址最大略大于 0x0000 5655 5555 5555

*/

