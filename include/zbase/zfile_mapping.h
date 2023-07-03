

/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

#pragma once
#ifndef _ZMAPPING_H_
#define _ZMAPPING_H_

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



//comment: grep -i commit /proc/meminfo   #查看overcommit value和当前commit   
//comment: sar -r											#查看kbcommit   %   
//comment: cat /proc/981/oom_score		#查看OOM分数  


//内存水位线
//watermark[min] = min_free_kbytes/4*zone.pages/zone.allpages    # cat /proc/sys/vm/min_free_kbytes    #32M    total/1000 左右,   通常在128k~65M之间  
//watermark[low] = watermark[min] * 5 / 4      
//watermark[high] = watermark[min] * 3 / 2

//当前水位线  cat /proc/zoneinfo | grep -E "Node|min|low|high "       #注意是页;  一般4k   每个zone都有自己的水位线   剩余内存超过HIGH代表内存剩余较多.  

//other   
//overcommit :   page table
//filesystem: page cache & buffer cache   
//kswapd: 水位线  
//内存不足会触发内存回收.   



class zfile_mapping_win32
{
public:
	zfile_mapping_win32()
	{
#ifdef WIN32
		mapping_hd_ = NULL;
		file_hd_ = NULL;
#endif
		file_data_ = NULL;
		file_size_ = 0;
	}
	~zfile_mapping_win32()
	{
		unmap_res();
	}

	s32 mapping_res(const char* file_path, bool readonly, bool remapping = false)
	{
#ifdef WIN32
		s32 ret = 0;

		if (remapping)
		{
			unmap_res();
		}

		if (file_hd_ != NULL)
		{
			return 1;
		}
		if (readonly)
		{
			file_hd_ = ::CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		else
		{
			file_hd_ = ::CreateFile(file_path, GENERIC_READ| GENERIC_WRITE, FILE_SHARE_READ| FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		
		if (file_hd_ == INVALID_HANDLE_VALUE)
		{
			return 1;
		}
		LARGE_INTEGER file_size;
		if (!GetFileSizeEx(file_hd_, &file_size))
		{
			::CloseHandle(file_hd_);
			file_hd_ = NULL;
			return 2;
		}

		if (readonly)
		{
			mapping_hd_ = CreateFileMapping(file_hd_, NULL, PAGE_READONLY, 0, 0, NULL);
		}
		else
		{
			mapping_hd_ = CreateFileMapping(file_hd_, NULL, PAGE_READWRITE, 0, 0, NULL);
		}
		
		if (mapping_hd_ == NULL)
		{
			::CloseHandle(file_hd_);
			file_hd_ = NULL;
			return 2;
		}

		if (readonly)
		{
			file_data_ = (char*)::MapViewOfFile(mapping_hd_, FILE_MAP_READ, 0, 0, 0);
		}
		else
		{
			file_data_ = (char*)::MapViewOfFile(mapping_hd_, FILE_MAP_READ| FILE_MAP_WRITE, 0, 0, 0);
		}

		
		if (file_data_)
		{
			file_size_ = (s64)file_size.QuadPart;
		}
		else
		{
			::CloseHandle(file_hd_);
			::CloseHandle(mapping_hd_);
			file_hd_ = NULL;
			mapping_hd_ = NULL;
			return 3;
		}
#endif
		return 0;
	}

	s32 is_mapped()
	{
#ifdef WIN32
		return file_hd_ != NULL;
#else
		return 0;
#endif
	}

	s32 unmap_res()
	{
#ifdef WIN32
		if (file_hd_ == 0)
		{
			return 1;
		}

		if (file_data_)
		{
			::UnmapViewOfFile(file_data_);
		}
		::CloseHandle(mapping_hd_);
		::CloseHandle(file_hd_);
		file_data_ = NULL;
		file_size_ = 0;
		mapping_hd_ = NULL;
		file_hd_ = NULL;
#endif
		return 0;
	}

	s32 remove_file(const char* file_path)
	{
		return ::remove(file_path);
	}

	s32 trim_cache()
	{
		return 0;
	}
public:
	const char* file_data() { return file_data_; }
	s64 file_size() { return file_size_; }
private:
#ifdef WIN32
	HANDLE file_hd_;
	HANDLE mapping_hd_;
#endif
	char* file_data_;
	s64   file_size_;
};





class zfile_mapping_unix
{

public:
	zfile_mapping_unix()
	{
		file_fd_ = -1;
		file_data_ = NULL;
		file_size_ = 0;
	}
	~zfile_mapping_unix()
	{
		unmap_res();
	}


	s32 mapping_res(const char* file_path, bool readonly, bool remapping = false)
	{
#ifndef WIN32	
		if (remapping)
		{
			unmap_res();
		}
		if (file_fd_ != -1)
		{
			return 1;
		}

		struct stat sb;
		if (readonly)
		{
			file_fd_ = open(file_path, O_RDONLY);
		}
		else
		{
			file_fd_ = open(file_path, O_RDWR);
		}
		if (file_fd_ == -1)
		{
			return 10;
		}

		if (fstat(file_fd_, &sb) == -1)
		{
			close(file_fd_);
			file_fd_ = -1;
			return 11;
		}
		if (readonly)
		{
			file_data_ = (char*)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, file_fd_, 0);
		}
		else
		{
			file_data_ = (char*)mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd_, 0);
		}
		file_size_ = sb.st_size;
#endif 
		return 0;
	}



	s32 is_mapped()
	{
		return file_fd_ != -1;
	}

	s32 unmap_res()
	{
		if (file_fd_ == 0)
		{
			return 1;
		}
#ifndef WIN32
		if (file_data_ != NULL && file_size_ != 0)
		{
			munmap(file_data_, file_size_);
		}
		close(file_fd_);
		file_data_ = NULL;
		file_size_ = 0;
		file_fd_ = -1;
#endif // 0
		return 0;
	}
	s32 remove_file(const char* file_path)
	{
		return ::remove(file_path);
	}
	s32 trim_cache()
	{
		if (!is_mapped())
		{
			return 1;
		}
#if !defined(__APPLE__) && !defined(WIN32) 
		fsync(file_fd_);
		posix_fadvise(file_fd_, 0, 0, POSIX_FADV_DONTNEED);
		fsync(file_fd_);
#endif
		return 0;
	}
public:
	const char* file_data() { return file_data_; }
	s64 file_size() { return file_size_; }
private:
	int file_fd_;
	char* file_data_;
	s64   file_size_;
};


/* type_traits:
*
* is_trivially_copyable: no
	* memset: no
	* memcpy: no
* shm resume : no
	* has vptr:     no
	* static var:   no
	* has heap ptr: no
	* has code ptr: no
	* has sys ptr: yes
* thread safe: no
*
*/

class zfile_mapping
{
#ifdef WIN32
	zfile_mapping_win32 mapping_;
#else
	zfile_mapping_unix mapping_;
#endif // WIN32

public:
	zfile_mapping()
	{
	}
	~zfile_mapping()
	{
	}

	const char* file_data() { return mapping_.file_data(); }
	s64 file_size() { return mapping_.file_size(); }

	s32 mapping_res(const char* file_path, bool readonly = true, bool remapping = false)
	{
		return mapping_.mapping_res(file_path, readonly, remapping);
	}

	s32 is_mapped()
	{
		return mapping_.is_mapped();
	}

	s32 unmap_res()
	{
		return mapping_.unmap_res();
	}

	s32 remove_file(const char* file_path)
	{
		return mapping_.remove_file(file_path);
	}

	s32 trim_cache()
	{
		return mapping_.trim_cache();
	}
};




#endif