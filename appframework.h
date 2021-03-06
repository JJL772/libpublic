/*
appframework.h - Definitions for the appframework system
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

#include "containers/list.h"

/* Make sure these dumbos are not here */
#undef min
#undef max
#include <initializer_list>
#include <functional>
#include "public.h"

/**
 * AppFramework and how it works
 *
 * This is a system that's heavily modeled off of Source 1's AppFramework system.
 * There are some pretty major differences, however.
 *
 * The base class IModuleInterface should be the parent of any exported interface.
 * A module can export an infinite number of interfaces.
 *
 * Each interface has a couple of properties:
 * 	- Version number
 * 	- Parent interface
 *
 * The parent interface is the interface in which it implements. E.g. IFilesystem001
 * Version numbers are rather abstract, and they're a part of the interface parent name itself (e.g. 001 in the above example)
 *
 * Interfaces are loaded into interface "slots". Basically you have an interface implementation, such as IEngineFilesystem001, which
 * is an implementation of IFilesystem001. So, when you load IEngineFilesystem001, it's loaded into the IFilesystem001 slot. When other modules lookup
 * interfaces, they refer to the "slot" name, in this case IFilesystem001, and the IEngineFilesystem001 implementation is returned.
 *
 * Implementation NOTES:
 *  - AppFramework acts as a state machine, so it should only be called from the main thread
 *  - There are some pretty terrible hacks in here, which does make me somewhat sad
 */

class IAppInterface
{
public:
	virtual const char* GetParentInterface() = 0;

	virtual const char* GetName() = 0;

	virtual bool PreInit() = 0;

	virtual bool Init() = 0;

	virtual void Shutdown() = 0;
};

typedef struct
{
	const char* name;
	const char* parent;
} iface_t;

/* Returns a list of interfaces exported by this module */
/* The single int parameter should be set to the number of interfaces in the returned array */
EXPORT typedef iface_t* (*pfnGetInterfaces)(int*);

/* Returns a new pointer to the specified interface */
/* First parameter is the interface name (not the parent!) and the second is an out parameter for the status code */
EXPORT typedef void* (*pfnCreateInterface)(const char*, int*);

/* Used for the iface status parameter in pfnCreateInterface */
enum EIfaceStatus
{
	OK     = 0,
	FAILED = 1,
};

/*

 API usage:

 AppFramework_AddInterface("engine.so", "IFilesystem001");
 AppFramework_AddInterface("engine.so", "IEngineTrace001");

 AppFramework_LoadInterfaces();

 void* ptr = AppFramework_FindInterface("IEngineTrace001");

 ...

 */

namespace AppFramework
{
struct interface_t
{
	const char* module;
	const char* iface;
};

/**
 * @brief Adds an interface to the load list
 * @param module shared object or DLL this comes from
 * @param iface Interface name
 * @return True if added, false otherwise
 */
EXPORT bool AddInterface(const char* module, const char* iface);

EXPORT bool AddInterfaces(std::initializer_list<interface_t> interfaces);
EXPORT bool AddInterfaces(interface_t* interfaces);

/**
 * @brief Gets the last error that happened or empty string
 * @return last error
 */
EXPORT const char* GetLastError();

/**
 * @brief Called to load all interfaces queued for load
 * @return True if OK, false if something failed
 */
EXPORT bool LoadInterfaces();

/**
 * @brief Finds a pointer to a loaded interface
 * @param iface Name of the interface
 * @return Pointer to the iface or NULL if not found
 */
EXPORT void* FindInterface(const char* iface);

/**
 * @brief Unloads all added interfaces
 */
EXPORT void UnloadInterfaces();

/**
 * Set a custom LoadLibrary/FreeLibrary function
 */
EXPORT void SetLoadLibrary(std::function<void*(const char*)> fn);
EXPORT void SetFreeLibrary(std::function<void(void*)> fn);
EXPORT void ClearCustomFunctions();

/**
 * Utility class that maintains a handle to an arbitrary AppSystem.
 * The handle returned from this is guaranteed to be valid, as SIGABRT will be raised if the interface is not found
 *
 * USAGE:
 * 	CAppSystemHandle<ILoggingSystem> g_LoggingSystem(IENGINELOGGING_INTERFACE);
 * 	g_LoggingSystem.Get().Log(....);
 */
template <class T> class CAppSystemHandle
{
public:
	CAppSystemHandle(const char* iface_name);

	T&	 Get();
	const T& Get() const;

	void Load();
	bool IsLoaded();
};
} // namespace AppFramework

/*
=======================

 Utility macros

=======================
*/

#define EXPOSE_INTERFACE(_int)                                                                                                                       \
	extern List<IAppInterface*>* g_pInterfaces;                                                                                                  \
	class __CStaticWrapperForInterfaces_##_int                                                                                                   \
	{                                                                                                                                            \
	public:                                                                                                                                      \
		__CStaticWrapperForInterfaces_##_int()                                                                                               \
		{                                                                                                                                    \
			if (!g_pInterfaces)                                                                                                          \
				g_pInterfaces = new List<IAppInterface*>();                                                                          \
			g_pInterfaces->push_back(new _int());                                                                                        \
		}                                                                                                                                    \
	};                                                                                                                                           \
	static __CStaticWrapperForInterfaces_##_int g_##_int##_InterfaceGlobal;

/* For the CreateInterface impl */
#define MODULE_INTERFACE_IMPL()                                                                                                                      \
	List<IAppInterface*>*	g_pInterfaces;                                                                                                       \
	extern "C" EXPORT void* CreateInterface(const char* name, int* retcode)                                                                      \
	{                                                                                                                                            \
		if (!g_pInterfaces)                                                                                                                  \
		{                                                                                                                                    \
			*retcode = (int)EIfaceStatus::FAILED;                                                                                        \
			return nullptr;                                                                                                              \
		}                                                                                                                                    \
		for (auto x : *g_pInterfaces)                                                                                                        \
		{                                                                                                                                    \
			if (strcmp(x->GetName(), name) == 0)                                                                                         \
			{                                                                                                                            \
				*retcode = (int)EIfaceStatus::OK;                                                                                    \
				return x;                                                                                                            \
			}                                                                                                                            \
		}                                                                                                                                    \
		*retcode = (int)EIfaceStatus::FAILED;                                                                                                \
		return 0;                                                                                                                            \
	}                                                                                                                                            \
	extern "C" EXPORT iface_t* GetInterfaces(int* num)                                                                                           \
	{                                                                                                                                            \
		static iface_t* __iface_list = 0;                                                                                                    \
		if (!__iface_list)                                                                                                                   \
			__iface_list = new iface_t[g_pInterfaces->size()];                                                                           \
		int i = 0;                                                                                                                           \
		for (auto x : *g_pInterfaces)                                                                                                        \
		{                                                                                                                                    \
			__iface_list[i] = iface_t{x->GetName(), x->GetParentInterface()};                                                            \
			i++;                                                                                                                         \
		}                                                                                                                                    \
		*num = g_pInterfaces->size();                                                                                                        \
		return __iface_list;                                                                                                                 \
	}
