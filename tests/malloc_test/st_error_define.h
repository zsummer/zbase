#pragma once

enum STAllocErrorDef
{
	ST_ALLOC_ERROR_SUCCESS = 0, // 没有任何错误
	ST_ALLOC_ERROR_ALREADY_INITED = 1,  // 已经初始化过了
	ST_ALLOC_ERROR_INVALID_PARAMS = 2,  // 无效参数
	ST_ALLOC_ERROR_UNINITED = 3,  // 未初始化
	ST_ALLOC_SHM_NOT_ALLOCATED = 4, // 共享内存还未分配出来
	ST_ALLOC_SHM_INFO_NOT_MATCH = 5, // 共享内存信息不匹配
	ST_ALLOC_SHM_BUDDY_ERROR = 6, // 伙伴系统出错了
	ST_ALLOC_SHM_META_ALLOC_FAILED = 7, // 元数据分配失败
};