
/*
* zfile License
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

#ifndef  ZFILE_H
#define ZFILE_H

#include <type_traits>
#include <iterator>
#include <cstddef>
#include <memory>
#include <algorithm>

#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <iostream>



#ifdef WIN32
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
#include <stdlib.h>
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




class zfile
{
public:
    inline zfile();
    inline ~zfile();
    inline bool is_open();
    
    inline long open(const char* path, const char* mod, struct stat& file_stat);
    inline void close();
    inline void write(const char* data, size_t len);
    inline void flush();
    inline std::string read_line();
    inline std::string read_content();

    static inline bool is_dir(const std::string& path);
    static inline bool is_file(const std::string& path);
    static inline bool create_dir(const std::string& path);
    static inline std::string process_id();
    static inline std::string process_name();
    static inline bool remove_file(const std::string& path);
    static inline struct tm time_to_tm(time_t t);
    static inline s64 file_size(const std::string& path);
    static inline std::string file_content(const std::string& path, const char* mod);
    static inline std::string file_bin_content(const std::string& path) { return file_content(path, "rb"); }
    static inline std::string file_text_content(const std::string& path) { return file_content(path, "r"); }
public:
    FILE* file_;
};




/*
"r"
Opens a file for reading. The file must exist.

"w"
Creates an empty file for writing. If a file with the same name already exists, its content is erased and the file is considered as a new empty file.

"a"
Appends to a file. Writing operations, append data at the end of the file. The file is created if it does not exist.

"r+"
Opens a file to update both reading and writing. The file must exist.

"w+"
Creates an empty file for both reading and writing.

"a+"
Opens a file for reading and appending.

"b"

 The mode string can also include the letter 'b' either as a last
       character or as a character between the characters in any of the
       two-character strings described above.  This is strictly for
       compatibility with C89 and has no effect; the 'b' is ignored on
       all POSIX conforming systems, including Linux.  (Other systems
       may treat text files and binary files differently, and adding the
       'b' may be a good idea if you do I/O to a binary file and expect
       that your program may be ported to non-UNIX environments.)


*/

long zfile::open(const char* path, const char* mod, struct stat& file_stat)
{
    if (file_ != nullptr)
    {
        fclose(file_);
        file_ = nullptr;
    }
    file_ = fopen(path, mod);
    if (file_)
    {
        if (fstat(fileno(file_), &file_stat) != 0)
        {
            fclose(file_);
            file_ = nullptr;
            return -1;
        }
        return file_stat.st_size;
    }
    return -2;
}
void zfile::close()
{
    if (file_ != nullptr)
    {
#if !defined(__APPLE__) && !defined(WIN32) 
        if (file_ != nullptr)
        {
            int fd = fileno(file_);
            fsync(fd);
            posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
            fsync(fd);
        }
#endif
        fclose(file_);
        file_ = nullptr;
    }
}

zfile::zfile()
{
    file_ = nullptr;
}

zfile::~zfile()
{
    close();
}

bool zfile::is_open()
{
    return file_ != nullptr;
}

void zfile::write(const char* data, size_t len)
{
    if (file_ && len > 0)
    {
        if (fwrite(data, 1, len, file_) != len)
        {
            close();
        }
    }
}
void zfile::flush()
{
    if (file_)
    {
        fflush(file_);
    }
}

std::string zfile::read_line()
{
    char buf[500] = { 0 };
    if (file_ && fgets(buf, 500, file_) != nullptr)
    {
        return std::string(buf);
    }
    return std::string();
}


std::string zfile::read_content()
{
    std::string content;

    if (!file_)
    {
        return content;
    }
    char buf[BUFSIZ];
    size_t ret = 0;
    do
    {
        ret = fread(buf, sizeof(char), BUFSIZ, file_);
        content.append(buf, ret);
    } while (ret == BUFSIZ);

    return content;
}

bool zfile::is_dir(const std::string& path)
{
#ifdef WIN32
    return PathIsDirectoryA(path.c_str()) ? true : false;
#else
    DIR* pdir = opendir(path.c_str());
    if (pdir == nullptr)
    {
        return false;
    }
    else
    {
        closedir(pdir);
        pdir = nullptr;
        return true;
    }
#endif
}

bool zfile::is_file(const std::string& path)
{
#ifdef WIN32
    return ::_access(path.c_str(), 0) == 0;
#else
    return ::access(path.c_str(), F_OK) == 0;
#endif
}

bool zfile::create_dir(const std::string& path)
{
    if (path.length() == 0)
    {
        return true;
    }
    std::string sub;

    std::string::size_type pos = path.find('/');
    while (pos != std::string::npos)
    {
        std::string cur = path.substr(0, pos - 0);
        if (cur.length() > 0 && !is_dir(cur))
        {
            bool ret = false;
#ifdef WIN32
            ret = CreateDirectoryA(cur.c_str(), nullptr) ? true : false;
#else
            ret = (mkdir(cur.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
#endif
            if (!ret)
            {
                return false;
            }
        }
        pos = path.find('/', pos + 1);
    }

    return true;
}


std::string zfile::process_id()
{
    std::string pid = "0";
    char buf[260] = { 0 };
#ifdef WIN32
    DWORD winPID = GetCurrentProcessId();
    sprintf(buf, "%06u", winPID);
    pid = buf;
#else
    sprintf(buf, "%06d", getpid());
    pid = buf;
#endif
    return pid;
}

std::string zfile::process_name()
{
    std::string name = "process";
    char buf[260] = { 0 };
#ifdef WIN32
    if (GetModuleFileNameA(nullptr, buf, 259) > 0)
    {
        name = buf;
    }
    std::string::size_type pos = name.rfind("\\");
    if (pos != std::string::npos)
    {
        name = name.substr(pos + 1, std::string::npos);
    }
    pos = name.rfind(".");
    if (pos != std::string::npos)
    {
        name = name.substr(0, pos - 0);
    }

#elif defined(__APPLE__)
    proc_name(getpid(), buf, 260);
    name = buf;
    return name;;
#else
    sprintf(buf, "/proc/%d/cmdline", (int)getpid());
    zfile i;
    struct stat file_stat;
    i.open(buf, "rb", file_stat);
    if (!i.is_open())
    {
        return name;
    }
    name = i.read_line();
    i.close();

    std::string::size_type pos = name.rfind("/");
    if (pos != std::string::npos)
    {
        name = name.substr(pos + 1, std::string::npos);
    }
#endif

    return name;
}

bool zfile::remove_file(const std::string& path)
{
    return ::remove(path.c_str()) == 0;
}

struct tm zfile::time_to_tm(time_t t)
{
#ifdef WIN32
#if _MSC_VER < 1400 //VS2003
    return *localtime(&t);
#else //vs2005->vs2013->
    struct tm tt = { 0 };
    localtime_s(&tt, &t);
    return tt;
#endif
#else //linux
    struct tm tt = { 0 };
    localtime_r(&t, &tt);
    return tt;
#endif
}

s64 zfile::file_size(const std::string& path)
{
    struct stat statbuf;
    stat(path.c_str(), &statbuf);
    return statbuf.st_size;
}

std::string zfile::file_content(const std::string& path, const char* mod)
{
    zfile f;
    struct stat s;
    f.open(path.c_str(), mod, s);
    return f.read_content();
}

#endif