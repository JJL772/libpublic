#pragma once 
#include "allocator.h"

/* Standard includes */
#undef min
#undef max
#include <vector>

template<class T> 
class Array : public std::vector<T>
{

};
