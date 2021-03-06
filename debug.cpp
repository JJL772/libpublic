/*
debug.cpp - Debug stuff
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
#include "public.h"
#include "build.h"
#include "debug.h"
#include "threadtools.h"
#include "crtlib.h"
#include "containers/array.h"
#include "cmdline.h"

#include <signal.h>
#include <stdlib.h>
#include <memory.h>

using namespace dbg;

/* TODO: Organize this by file to speed up searches */
Array<CAssert>* g_passertions = nullptr;

CThreadMutex* g_passert_mutex = nullptr;

bool g_asserts_once    = false;
bool g_asserts_break   = false;
bool g_asserts_disable = false;

#ifdef XASH_LINUX
static void HandleAbort(int sig, siginfo_t* siginfo, void* pdat);
#endif

static void DbgInit()
{
	static bool binit = false;
	if (binit)
		return;

#ifdef XASH_LINUX
	if (GlobalCommandLine().Find("-debug-test"))
	{
		struct sigaction sabrt;
		memset(&sabrt, 0, sizeof(sabrt));
		sabrt.sa_sigaction = HandleAbort;
		sigaction(SIGABRT, &sabrt, &sabrt);
	}
#endif

	if (!g_passert_mutex)
		g_passert_mutex = new CThreadMutex();
	if (!g_passertions)
		g_passertions = new Array<CAssert>();
	binit = true;
}

void dbg::Init() { DbgInit(); }

/* Strips off the ../ that all files have appeneded to them */
static const char* _CleanName(const char* file)
{
	size_t sz = Q_strlen(file);
	if (sz > 3 && Q_startswith(file, ".." PATH_SEPARATOR))
	{
		return &file[3];
	}
	return file;
}

/* Internal functions (No locking done) */
static CAssert& _FindOrCreateAssert(const char* file, int line, const char* exp)
{
	const char* cleanName = _CleanName(file);
	for (auto& x : *g_passertions)
	{
		if (Q_strcmp(cleanName, x.File()) == 0 && line == x.m_line)
			return x;
	}
	CAssert assert(line, _CleanName(file), "");
	g_passertions->push_back(assert);
	return (*g_passertions)[g_passertions->size() - 1];
}

static CAssert& _CreateAssert(const char* file, int line, const char* exp)
{
	CAssert assert(line, _CleanName(file), exp);
	g_passertions->push_back(assert);
	return (*g_passertions)[g_passertions->size() - 1];
}

static CAssert* _FindAssert(const char* file, int line)
{
	const char* cleanName = _CleanName(file);
	for (auto& x : *g_passertions)
	{
		if (Q_strcmp(cleanName, x.m_file) == 0 && x.m_line == line)
			return &x;
	}
	return nullptr;
}

CAssert dbg::FindOrCreateAssert(const char* file, int line, const char* exp)
{
	DbgInit();
	auto lock = g_passert_mutex->RAIILock();
	return _FindOrCreateAssert(file, line, exp);
}

void dbg::DisableAssert(const char* file, int line)
{
	DbgInit();
	auto lock = g_passert_mutex->RAIILock();

	CAssert* ass = _FindAssert(file, line);
	if (ass)
		ass->m_ignored = true;
}

void dbg::EnableAssert(const char* file, int line)
{
	DbgInit();
	auto lock = g_passert_mutex->RAIILock();

	CAssert* ass = _FindAssert(file, line);
	if (ass)
		ass->m_ignored = false;
}

CAssert dbg::FindAssert(const char* file, int line)
{
	DbgInit();
	auto	 lock = g_passert_mutex->RAIILock();
	CAssert* ass  = _FindAssert(file, line);
	if (!ass)
		return CAssert(0, "", "");
	return *ass;
}

bool dbg::IsAssertEnabled(const char* file, int line)
{
	DbgInit();
	auto lock = g_passert_mutex->RAIILock();

	CAssert* ass = _FindAssert(file, line);
	if (!ass)
		return false;
	return !ass->m_ignored;
}

void dbg::CreateAssert(const char* file, int line, const char* exp)
{
	DbgInit();
	auto lock = g_passert_mutex->RAIILock();
	_CreateAssert(file, line, exp);
}

bool dbg::FireAssertion(const char* file, int line, const char* exp)
{
	DbgInit();
	auto lock = g_passert_mutex->RAIILock();

	CAssert& ass = _FindOrCreateAssert(file, line, exp);
	ass.m_timesHit++; // The assertion's hit counter will be marked each and every time, even if the assert is ignored
	ass.m_exp = exp;  // Just going to update the expression here as it might not be set before the assert is actually hit

	// Are asserts globally disabled?
	if (g_asserts_disable)
		return false;

	if ((ass.m_assertOnce || g_asserts_once) && ass.m_timesHit > 1)
		return false;

	// Is the assert broken or are all asserts broken?
	if (ass.m_break || g_asserts_break)
		raise(SIGINT);

	return !ass.m_ignored;
}

void dbg::BreakAssert(const char* file, int line)
{
	DbgInit();
	auto lock = g_passert_mutex->RAIILock();

	CAssert& ass = _FindOrCreateAssert(file, line, "");
	ass.m_break  = true;
}

void dbg::UnBreakAssert(const char* file, int line)
{
	DbgInit();
	auto lock = g_passert_mutex->RAIILock();

	CAssert& ass = _FindOrCreateAssert(file, line, "");
	ass.m_break  = false;
}

bool dbg::WasAssertHit(const char* file, int line)
{
	DbgInit();
	auto lock = g_passert_mutex->RAIILock();

	CAssert* ass = _FindAssert(file, line);
	if (!ass)
		return false;
	return ass->m_timesHit != 0;
}

Array<CAssert> dbg::GetAssertList()
{
	DbgInit();
	auto lock = g_passert_mutex->RAIILock();
	return *g_passertions;
}

void dbg::EnableAssertBreak() { g_asserts_break = true; }

void dbg::DisableAssertBreak() { g_asserts_break = false; }

void dbg::EnableAssertOnce() { g_asserts_once = true; }

void dbg::DisableAssertOnce() { g_asserts_once = false; }

void dbg::EnableAsserts() { g_asserts_disable = false; }

void dbg::DisableAsserts() { g_asserts_disable = true; }

CAssert::CAssert(int line, const char* file, const char* exp)
	: m_file(file), m_line(line), m_ignored(false), m_exp(exp), m_timesHit(0), m_break(false), m_assertOnce(false)
{
}

CAssert::~CAssert() {}

#ifdef XASH_LINUX
static void HandleAbort(int sig, siginfo_t* siginfo, void* pdat) { exit(1); }
#endif
