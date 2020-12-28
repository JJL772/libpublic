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

template <class T, class A = std::allocator<T>> class List : public std::list<T, A>
{
public:
	bool contains(const T& item)
	{
		for (auto x : *this)
			if (item == x)
				return true;
		return false;
	}

	void remove(const T& item)
	{
		for (auto it = this->begin(); it != this->end(); ++it)
		{
			if (*it == item)
			{
				this->erase(it);
				return;
			}
		}
	}
};
