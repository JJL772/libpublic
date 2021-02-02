/*
Copyright (C) 2020 Jeremy Lorelli

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#pragma once
#include "allocator.h"

/* Standard includes */
#undef min
#undef max
#include <vector>
#include <initializer_list>

template <class T> class Array : public std::vector<T>
{
public:
	Array(std::initializer_list<T> ls) :
		std::vector<T>(ls)
	{
	}

	Array(const T* p, size_t n)
	{
		for(size_t i = 0; i < n; i++) {
			this->push_back(p[i]);
		}
	}

	Array() :
		std::vector<T>()
	{
	}

	void remove(const T& t, size_t max = 1)
	{
		for(auto it = this->begin(); it != this->end() && max > 0; ++it) {
			if(*it == t) {
				max--;
				erase(it);
			}
		}
	}

	bool contains(const T& t)
	{
		for(const auto& x : *this) {
			if(x == t)
				return true;
		}
		return false;
	}

	void concat(const Array<T>& other)
	{
		for(const auto& x : other) {
			this->push_back(x);
		}
	}

	Array<T>& operator+=(const Array<T>& o)
	{
		concat(o);
		return *this;
	}
};
