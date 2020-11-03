/*
mem.cpp - zone memory allocation from DarkPlaces
Copyright (C) 2007 Uncle Mike
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
/*
 * Originally this code was located in the engine and initialized early on in Host_Main.
 * However, with the addition of tier1 to the engine, and since we've ported a bunch of stuff to C++,
 * static initialization is now a thing and it's used a LOT. Problem is, sometimes statically initialized objects
 * (such as cvars), depend on the engine's memory allocator internally, due to how the cvar system works.
 * Since the engine allocator isn't initialized like *as soon* as the program starts, we sometimes get engine crashes
 * with various tier1 utils, or other things. To fix this we've just moved the zone allocator here, which is a much better
 * solution overall.
 */

/**
 * CUSTOM MEMORY ALLOCATORS
 *	Custom allocators can be either standalone or based on IBaseMemoryAllocator.
 * 	In general, IBaseMemoryAllocator should be used as a base class if you're trying to create a
 * 	stock-alike allocator that only implements malloc, calloc, realloc and free and does not depend on
 * 	any extra info or behaviour.
 * 	Allocators that are special (See CSmallBlockAllocator), can be their own class type.
 * 	If you wish to use these allocators with standard containers, create a custom allocator in allocator.h
 *
 * THREAD SAFETY
 * 	Allocators should be 100% thread safe. Data allocated in one thread must be free-able by other threads too.
 * 	Custom allocators may not be thread safe, but that's OKAY.
 *
 * TEMPLATED ALLOCATORS
 * 	Templated allocators should be implemented in a header-only fashion if possible.
 * 	It might be best to hide internal logic inside of mem.cpp or some other source file.
 *
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#include "public.h"

/* Different from the other classes as we're trying to replace the engine's zone allocator */
class EXPORT CZoneAllocator
{
public:
	CZoneAllocator();
	virtual void Memory_Init( void );
	virtual void *_Mem_Realloc( byte *poolptr, void *memptr, size_t size, bool clear, const char *filename, int fileline );
	virtual void *_Mem_Alloc( byte *poolptr, size_t size, bool clear, const char *filename, int fileline );
	virtual byte *_Mem_AllocPool( const char *name, const char *filename, int fileline );
	virtual void _Mem_FreePool( byte **poolptr, const char *filename, int fileline );
	virtual void _Mem_EmptyPool( byte *poolptr, const char *filename, int fileline );
	virtual void _Mem_Free( void *data, const char *filename, int fileline );
	virtual void _Mem_Check( const char *filename, int fileline );
	virtual bool Mem_IsAllocatedExt( byte *poolptr, void *data );
	virtual void Mem_PrintList( size_t minallocationsize );
	virtual void Mem_PrintStats( void );
};

#ifdef LIBPUBLIC
EXPORT extern CZoneAllocator* g_pZoneAllocator;
#else
IMPORT extern CZoneAllocator* g_pZoneAllocator;
#endif

EXPORT CZoneAllocator& GlobalAllocator();

class EXPORT IBaseMemoryAllocator
{
public:
	virtual void* malloc(size_t sz) = 0;
	virtual void* calloc(size_t size_of_object, size_t num_objects) = 0;
	virtual void* realloc(void* ptr, size_t newsize) = 0;
	virtual void free(void* ptr) = 0;
};

/* TODO: Implement these new allocators */
#if 0
class CSmallBlockAllocator : public IBaseMemoryAllocator
{
public:
	void* malloc(size_t sz) override;
	void* calloc(size_t size_of_object, size_t num_objects) override;
	void* realloc(void* ptr, size_t newsize) override;
	void free(void* ptr)  override;
};

class CStringAllocator : public IBaseMemoryAllocator
{
public:
	void* malloc(size_t sz) override;
	void* calloc(size_t size_of_object, size_t num_objects) override;
	void* realloc(void* ptr, size_t newsize) override;
	void free(void* ptr)  override;
};

class CSlabAllocator : public IBaseMemoryAllocator
{
public:
	void* malloc(size_t sz) override;
	void* calloc(size_t size_of_object, size_t num_objects) override;
	void* realloc(void* ptr, size_t newsize) override;
	void free(void* ptr)  override;
};
#endif


/**
 * CSmallBlockAllocator is an allocator for a large number of small memory blocks. It does not
 * inherit from IBaseMemory allocator because it's designed to work with a single object type
 * of a fixed size and number.
 */
template<class T>
class EXPORT CSmallBlockAllocator
{
public:
	virtual void* malloc(size_t sz);
	virtual void* calloc(size_t size_of_object, size_t num_objects);
	virtual void* realloc(void* ptr, size_t newsize);
	virtual void free(void* ptr);
};
