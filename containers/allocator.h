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

#undef max
#undef min

#include <memory>
#include <cstdlib>
#include <exception>

template<class T>
class AllocatorBase
{
public:
	virtual T* allocate(std::size_t sz) { return nullptr; };
	virtual T* reallocate(T* ptr, std::size_t sz) { return nullptr; };
	virtual void deallocate(T* ptr) {};
};

template<class T>
class DefaultAllocator : public AllocatorBase<T>
{
public:
	T* allocate(std::size_t sz) override final
	{
		return (T*)std::malloc(sz);
	}

	T* reallocate(T* ptr, std::size_t sz) override final
	{
		return (T*)std::realloc(ptr, sz);
	}

	void deallocate(T* ptr) override final
	{
		std::free(ptr);
	}
};

template<class T, unsigned long long num>
class StaticAllocator : public AllocatorBase<T>
{
	T m_internalStore[num];
public:
	T* allocate(std::size_t sz) override final
	{
		return (T*)m_internalStore;
	}

	T* reallocate(T* ptr, std::size_t sz) override final
	{
		throw std::exception();
		return nullptr;
	}

	void deallocate(T* ptr) override final
	{
	}
};