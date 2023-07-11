#pragma once

#include "st_common.h"

// 代表连续的一批页面空间
struct Span
{
    PageID start; // 开始页面号
    Length page_num; // 页面数量
    Span* next;  // 双向链表
    Span* prev;  // 双向链表
    void* objects; // 分配的小对象链表头
    unsigned int refcount; // 已经分配的对象数量
    SizeClass sizeclass; // 小对象所属的class, 大对象此项为0
    //unsigned int location : 2; // 暂时不用
    //unsigned int sample : 1;

    // Span所在位置，暂时用不到
    //enum { IN_USE, ON_NORMAL_FREELIST, ON_RETURNED_FREELIST };
};

// 分配和销毁一个span, span管理的页面是外部分配的,span只负责跟踪
Span* NewSpan(PageID p, Length len);
void DeleteSpan(Span* span);

// span双向链表相关处理
void SpanDoubleListInit(Span* list);

// 从链表中删除一个Span
void SpanDoubleListRemove(Span* span);

// 判断链表是否为空
inline bool SpanDoubleListIsEmpty(const Span* list)
{
    return list->next == list;
}

// 在链表前面添加一个Span
void SpanDoubleListPrepend(Span* list, Span* span);

// 获取链表长度
int SpanDoubleListLength(const Span* list);
