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
#include "public.h"
#include "crtlib.h"
#include "cmdline.h"

CCommandLine::CCommandLine() :
	m_argc(0),
	m_argv(nullptr)
{

}

CCommandLine::~CCommandLine()
{

}

void CCommandLine::Set(int argc, char **argv)
{
	m_argc = argc;
	m_argv = argv;
}

bool CCommandLine::Find(const char *arg)
{
	for(int i = 0; i < m_argc; i++)
	{
		if(Q_strcmp(arg, m_argv[i]) == 0)
			return true;
	}
	return false;
}

const char *CCommandLine::FindString(const char *arg)
{
	for(int i = 0; i < m_argc; i++)
	{
		if(Q_strcmp(arg, m_argv[i]) == 0)
		{
			if(i < m_argc-1)
				return m_argv[i+1];
		}
	}
	return nullptr;
}

int CCommandLine::FindInt(const char *arg, int _default)
{
	for(int i = 0; i < m_argc; i++)
	{
		if(Q_strcmp(arg, m_argv[i]) == 0)
		{
			if(i < m_argc-1)
			{
				int out;
				if(Q_strint(m_argv[i+1], out))
					return out;
				else
					return _default;
			}
		}
	}
	return _default;
}

float CCommandLine::FindFloat(const char *arg, float _default)
{
	for(int i = 0; i < m_argc; i++)
	{
		if(Q_strcmp(arg, m_argv[i]) == 0)
		{
			if(i < m_argc-1)
			{
				float out;
				if(Q_strfloat(m_argv[i+1], out))
					return out;
				else
					return _default;
			}
		}
	}
	return _default;
}

EXPORT CCommandLine& GlobalCommandLine()
{
	static CCommandLine g_cmdline;
	return g_cmdline;
}
