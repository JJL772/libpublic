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

#include "public/containers/allocator.h"

/* Standard includes */
#undef min
#undef max
#include <unordered_map>
#include <map>

template<class KeyT, class ValT>
class Map : public std::map<KeyT, ValT>
{
public:
	void add(const KeyT& k, const ValT& v)
	{
		this->insert(std::pair<KeyT,ValT>(k,v));
	}
};

template<class KeyT, class ValT>
class MultiMap : public std::multimap<KeyT, ValT>
{
public:
	void add(const KeyT& k, const ValT& v)
	{
		this->insert(std::pair<KeyT,ValT>(k,v));
	}
};

template<class KeyT, class ValT, class Hash = std::hash<KeyT>>
class HashMap : public std::unordered_map<KeyT, ValT>
{
public:
	void add(const KeyT& k, const ValT& v)
	{
		this->insert(std::pair<KeyT,ValT>(k,v));
	}
};

template<class KeyT, class ValT, class Hash = std::hash<KeyT>>
class HasMultiMap
{
public:
	void add(const KeyT& k, const ValT& v)
	{
		this->insert(std::pair<KeyT,ValT>(k,v));
	}
};