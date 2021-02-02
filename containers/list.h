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
#include <list>
#include <initializer_list>

template <class T, class A = std::allocator<T>> class List : public std::list<T, A>
{
public:
	List(std::initializer_list<T> ls) :
		std::list<T,A>(ls)
	{

	}

	List(const T* p, size_t n)
	{
		for(size_t i = 0; i < n; i++) {
			push_back(p[i]);
		}
	}

	bool contains(const T& item)
	{
		for (auto x : *this)
			if (item == x)
				return true;
		return false;
	}

	void concat(const List<T,A>& other)
	{
		for(const auto& x : *this) {
			push_back(x);
		}
	}

	List<T,A>& operator+=(const List<T,A>& o)
	{
		concat(o);
		return *this;
	}

};
