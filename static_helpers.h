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

#include <functional>

/**
 * Calls a function during static initialization
 * @tparam fn
 * @tparam T
 */
template<void(*fn)()>
class CStaticInitWrapper
{
public:
	CStaticInitWrapper()
	{
		fn();
	}

	~CStaticInitWrapper()
	{
	}
};

class CLambdaStaticInitWrapper
{
public:
	CLambdaStaticInitWrapper(void(*fn)())
	{
		fn();
	}

	~CLambdaStaticInitWrapper()
	{

	}
};

template<void(*fn)()>
class CStaticDestructionWrapper
{
public:
	CStaticDestructionWrapper()
	{
	}

	~CStaticDestructionWrapper()
	{
		fn();
	}
};


template<void(*INIT)(), void(*SHUTDOWN)()>
class CStaticInitDestroyWrapper
{
public:
	CStaticInitDestroyWrapper()
	{
		INIT();
	}

	~CStaticInitDestroyWrapper()
	{
		SHUTDOWN();
	}
};

#define CallDuringStaticInit(x) static auto __static__ ## x ## __caller = CStaticInitWrapper<x>();