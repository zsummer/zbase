#pragma once
#include <stddef.h>
#include "st_common.h"
inline void* SLL_Next(void* t)
{
    return *(reinterpret_cast<void**>(t));
}

inline void SLL_SetNext(void* t, void* n)
{
    *(reinterpret_cast<void**>(t)) = n;
}

inline void SLL_Push(void** list, void* element)
{
    SLL_SetNext(element, *list);
    *list = element;
}

inline void* SLL_Pop(void **list)
{
    void* result = *list;
    *list = SLL_Next(*list);
    return result;
}

inline void SLL_PopRange(void** head, int N, void** start, void** end)
{
    if (N == 0)
    {
        *start = NULL;
        *end = NULL;
        return;
    }

    void* tmp = *head;
    for (int i = 1; i < N; ++i)
    {
        tmp = SLL_Next(tmp);
    }
    *start = *head;
    *end = tmp;
    *head = SLL_Next(tmp);
    SLL_SetNext(tmp, NULL);
}

inline void SLL_PushRange(void** head, void* start, void* end)
{
    if (!start)
    {
        return;
    }
    SLL_SetNext(end, *head);
    *head = start;
}

inline size_t  SLL_Size(void* head)
{
    int count = 0;
    while (head)
    {
        count++;
        head = SLL_Next(head);
    }
    return count;
}
