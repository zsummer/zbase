#include <string.h>

#include "st_common.h"
#include "span.h"
#include "stmalloc.h"

Span* NewSpan(PageID p, Length len)
{
    if (NULL == g_st_malloc)
    {
        error_tlog("stmalloc not init.");
        return NULL;
    }

    Span* result = g_st_malloc->span_allocator().New();
    if (!result)
    {
        return NULL;
    }

    memset(result, 0, sizeof(*result));
    result->start = p;
    result->page_num = len;

    return result;
}

void DeleteSpan(Span* span)
{
    if (NULL == g_st_malloc)
    {
        error_tlog("stmalloc not init.");
        return;
    }

    g_st_malloc->span_allocator().Delete(span);
}

void SpanDoubleListInit(Span* list)
{
    if (NULL == list)
    {
        return;
    }

    list->next = list;
    list->prev = list;
}

void SpanDoubleListRemove(Span* span)
{
    if (NULL == span)
    {
        return;
    }

    span->prev->next = span->next;
    span->next->prev = span->prev;

    span->prev = NULL;
    span->next = NULL;
}

int SpanDoubleListLength(const Span* list)
{
    if (NULL == list)
    {
        return 0;
    }

    int result = 0;
    for (Span* s = list->next; s != list; s = s->next)
    {
        ++result;
    }
    return result;
}

void SpanDoubleListPrepend(Span* list, Span* span)
{
    if (NULL == list || NULL == span)
    {
        return;
    }

    span->next = list->next;
    span->prev = list;
    list->next->prev = span;
    list->next = span;
}