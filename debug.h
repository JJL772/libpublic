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
#pragma once

#include "build.h"
#include "containers/array.h"
#include "public.h"

#define BEGIN_DBG_NAMESPACE namespace dbg {
#define END_DBG_NAMESPACE }

BEGIN_DBG_NAMESPACE

EXPORT void Init();

class EXPORT CAssert
{
public:
	int m_line;
	const char* m_file;
	const char* m_exp;
	bool m_ignored : 1;
	bool m_break : 1;
	bool m_assertOnce : 1;
	int m_timesHit;

public:
	CAssert(int line, const char* file, const char* exp);
	~CAssert();

	bool Enabled() const { return !m_ignored; }
	const char* File() const { return m_file; }
	int Line() const { return m_line; }
};

/* Called to fire an assertion. Returns true if the assertion is enabled and a message should be printed */
EXPORT bool FireAssertion(const char* file, int line, const char* exp);

/* Creates a new assertion */
EXPORT void CreateAssert(const char* file, int line, const char* exp);

/* Finds or creates a new assertion, always returns a valid pointer */
EXPORT CAssert FindOrCreateAssert(const char* file, int line, const char* exp);

/* Disables the assertion at the specified line and file */
EXPORT void DisableAssert(const char* file, int line);
EXPORT void EnableAssert(const char* file, int line);

/* Tries to find an assertion that's already registered. Returns nullptr if not found */
EXPORT CAssert FindAssert(const char* file, int line);

/* Returns true if an assertion is enabled */
EXPORT bool IsAssertEnabled(const char* file, int line);

/* Causes a break into debugger when the specified assertion is hit */
EXPORT void BreakAssert(const char* file, int line);
EXPORT void UnBreakAssert(const char* file, int line);

/* Returns true if the assertion was hit */
EXPORT bool WasAssertHit(const char* file, int line);

/* Returns a list of assertions */
EXPORT Array<CAssert> GetAssertList();

/* Enable/Disable features globally */
/* These act on ALL asserts */
EXPORT void EnableAssertOnce();
EXPORT void DisableAssertOnce();
EXPORT void EnableAssertBreak();
EXPORT void DisableAssertBreak();
EXPORT void EnableAsserts();
EXPORT void DisableAsserts();

END_DBG_NAMESPACE
