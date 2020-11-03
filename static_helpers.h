/*
 *
 * static_helpers.h - Helpers for use with static initialization
 *
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