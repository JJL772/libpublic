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
#include <set>
#include <initializer_list>

template<class T, class C = std::less<T>, class A = std::allocator<T>>
class Set : public std::set<T, C, A>
{
public:
	Set(const T* p, size_t n)
	{
		for(size_t i = 0; i < n; i++) {
			insert(p[i]);
		}
	}

	Set(std::initializer_list<T> ls) :
		std::set<T,C,A>(ls)
	{

	}

	inline bool contains(const T& e) const
	{
		return this->find(e) != this->end();
	}

	/* Set intersection with other */
	/* WARNING: SLOW */
	void intersect(const Set<T,C,A> &other)
	{
		Set<T,C,A> tmp;
		for(const auto& x : *this) {
			if(other.contains(x)) {
				tmp.insert(x);
			}
		}
		this->clear();
		merge(tmp);
	}

	/* Set union */
	void unify(const Set<T,C,A>& other)
	{
		merge(other);
	}
};
