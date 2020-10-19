/*
 *
 * tlist.h - Thread-safe lockless list
 *
 */
#pragma once

#include "public/containers/allocator.h"
#include "threadtools.h"

/* Standard includes */
#undef min
#undef max

/* Singly linked thread-safe list */
template<class T, class A = DefaultAllocator<T>>
class TSList
{
private:

public:
};
