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
/**
 * logger.h
 * 	Generic, thread-safe logging system
 */
#pragma once

#include "public.h"
#include "containers/list.h"
#include "containers/string.h"

#define COLOR_NORMAL "^1"
#define COLOR_RED    "^2"
#define COLOR_GREEN  "^3"
#define COLOR_BLUE   "^4"
#define COLOR_PURPLE "^5"
#define COLOR_YELLOW "^6"
#define COLOR_BOLD   "^7"
#define COLOR_STRIKE "^8"

namespace logger
{
EXPORT void Printf(const char* fmt, ...);
EXPORT void Errorf(const char* fmt, ...);
EXPORT void Warnf(const char* fmt, ...);
} // namespace logger

struct LogColor
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

using LogChannel = unsigned int;
using LogGroup	 = unsigned int;
using LogBackend = unsigned int;

enum class ELogLevel
{
	MSG_DEBUG = 0,
	MSG_GENERAL,
	MSG_WARN,
	MSG_ERROR,
	MSG_FATAL
};

struct LogChannelDescription_t
{
	String		 name;
	LogColor	 defaultColor;
	List<LogGroup>	 groups;
	List<LogBackend> backends;
};

class ILogBackend
{
public:
	virtual void Log(LogChannel chan, ELogLevel lvl, LogColor color, const char* msg) = 0;

	bool m_enabledForAll = false;
};

namespace Log
{
static constexpr LogChannel INVALID_CHANNEL_ID = -1u;
static constexpr LogGroup   INVALID_GROUP_ID   = -1u;
static constexpr LogBackend INVALID_BACKEND_ID = -1u;

static constexpr LogChannel GENERAL_CHANNEL_ID = 0;

/**
 * Returns the description for a specific logging channel
 * @param chan
 * @return
 */
const LogChannelDescription_t* GetChannelDescription(LogChannel chan);

LogChannel GetChannelByName(const char* name);

LogChannel CreateChannel(const char* name, LogColor color);

LogBackend   RegisterBackend(ILogBackend* backend);
ILogBackend* GetBackend(LogBackend backend);
void	     ClearBackends();

void DisableBackendForChannel(LogChannel chan, LogBackend backend);
void EnableBackendForChannel(LogChannel chan, LogBackend backend);
void DisableBackend(LogBackend backend);
void EnableBackend(LogBackend backend);

void Log(LogChannel chan, ELogLevel level, LogColor color, const char* fmt, ...);
void Log(LogChannel chan, ELogLevel level, LogColor color, const char* fmt, va_list list);
void Log(LogChannel chan, ELogLevel level, const char* fmt, ...);
void Log(LogChannel chan, ELogLevel level, const char* fmt, va_list list);

/* Log channel groups */
LogGroup CreateGroup(const char* name);
void	 AddChannelToGroup(LogChannel chan, LogGroup group);
void	 RemoveChannelFromGroup(LogChannel chan, LogGroup group);
bool	 IsChannelInGroup(LogChannel chan, LogGroup group);
int	 NumChannelsInGroup(LogGroup grp);

/**
 * Basic logging backend, supports colors
 */
class DefaultLogBackend : public ILogBackend
{
private:
	bool m_colorized = false;

protected:
	void SetStreamColor(unsigned char r, unsigned char g, unsigned char b);
	void ResetStreamColor();
	void SetStreamBold();

public:
	explicit DefaultLogBackend(bool colorized = true);

	void Log(LogChannel chan, ELogLevel lvl, LogColor color, const char* msg) override;
};

/**
 * File log backend using STDIO functions
 */
class StdFileLogBackend : public ILogBackend
{
private:
	const char* m_path;
	FILE*	    m_stream;

public:
	explicit StdFileLogBackend(const char* path);
	~StdFileLogBackend();

	void Log(LogChannel chan, ELogLevel lvl, LogColor color, const char* msg) override;
};
} // namespace Log