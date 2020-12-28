/*
cmdline.cpp - Commandline implementation
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

#include "public.h"

class EXPORT CCommandLine
{
private:
	char** m_argv;
	int    m_argc;
	CCommandLine();
	~CCommandLine();
	friend EXPORT CCommandLine& GlobalCommandLine();

public:
	void Set(int argc, char** argv);

	bool	    Find(const char* arg);
	const char* FindString(const char* arg);
	int	    FindInt(const char* arg, int _default = 0);
	float	    FindFloat(const char* arg, float _default = 0);

	int		   ArgCount() const { return m_argc; }
	int		   argc() const { return m_argc; };
	const char* const* argv() const { return m_argv; }
	const char* const* Args() const { return m_argv; }
};

EXPORT CCommandLine& GlobalCommandLine();
