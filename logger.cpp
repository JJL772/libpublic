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
#include "logger.h"
#include "containers/array.h"
#include "threadtools.h"
#include "containers/buffer.h"
#include "crtlib.h"

void logger::Printf(const char *fmt, ...)
{

}

void logger::Errorf(const char *fmt, ...)
{

}

void logger::Warnf(const char *fmt, ...)
{

}

struct LogGroupDesc_t
{
	String name;
};

/* GLBOAL ACCESSORS */
CThreadRecursiveMutex* GlobalLogMutex()
{
	static CThreadRecursiveMutex gMut;
	return &gMut;
}

Array<LogChannelDescription_t>& GlobalChannelList()
{
	static Array<LogChannelDescription_t> gChannels;
	/* init channels list with the channels we need */
	if(gChannels.empty())
	{
		auto lock = GlobalLogMutex()->RAIILock();

		LogChannelDescription_t generalDesc;
		generalDesc.defaultColor = {255, 255, 255};
		generalDesc.name = "General";
		gChannels.push_back(generalDesc);

		/* Add the default logging listener */
		Log::DefaultLogBackend* backend = new Log::DefaultLogBackend(true);
		backend->m_enabledForAll = true;
		Log::RegisterBackend(backend);
	}
	return gChannels;
}

Array<ILogBackend*>& GlobalBackendList()
{
	static Array<ILogBackend*> gBackends;
	return gBackends;
}

Array<LogGroup>& GlobalGroupList()
{
	static Array<LogGroup> gGroups;
	return gGroups;
}


static LogChannelDescription_t* GetChanDesc(LogChannel chan)
{
	auto lock = GlobalLogMutex()->RAIILock();

	auto& channels = GlobalChannelList();
	return &channels[chan];
}

static inline bool CheckChannelId(LogChannel chan)
{
	return (chan != Log::INVALID_CHANNEL_ID) && (chan < GlobalChannelList().size());
}

static inline bool CheckGroupId(LogGroup grp)
{
	return (grp != Log::INVALID_GROUP_ID) && (grp < GlobalGroupList().size());
}

static inline bool CheckBackendId(LogBackend b)
{
	return (b != Log::INVALID_BACKEND_ID) && (b < GlobalBackendList().size());
}

const LogChannelDescription_t* Log::GetChannelDescription(LogChannel chan)
{
	return GetChanDesc(chan);
}

LogChannel Log::GetChannelByName(const char* name)
{
	auto lock = GlobalLogMutex()->RAIILock();

	auto& channels = GlobalChannelList();
	int i = 0;
	for(auto it = channels.begin(); it != channels.end(); ++it, i++)
	{
		if(it->name.equals(name))
			return i;
	}
	return INVALID_CHANNEL_ID;
}


LogChannel Log::CreateChannel(const char* name, LogColor color)
{
	auto lock = GlobalLogMutex()->RAIILock();
	auto& channels = GlobalChannelList();

	/* if already registered, just return the existing channel */
	for(int i = 0; i < channels.size(); i++)
	{
		if(channels[i].name.equals(name))
			return i;
	}


	LogChannelDescription_t chan;
	chan.name = name;
	chan.defaultColor = color;

	auto& backendlist = GlobalBackendList();
	for(LogChannel c = 0; c < backendlist.size(); c++)
	{
		if(backendlist[c]->m_enabledForAll)
			chan.backends.push_back(c);
	}

	channels.push_back(chan);
	return channels.size()-1;
}


LogBackend Log::RegisterBackend(ILogBackend* backend)
{
	auto lock = GlobalLogMutex()->RAIILock();

	auto& backends = GlobalBackendList();

	backends.push_back(backend);
	return backends.size()-1;
}

ILogBackend* Log::GetBackend(LogBackend backend)
{
	if(!CheckBackendId(backend))
		return nullptr;

	auto lock = GlobalLogMutex()->RAIILock();

	auto& backends = GlobalBackendList();

	return backends[backend];
}

void Log::ClearBackends()
{
	auto lock = GlobalLogMutex()->RAIILock();

	GlobalBackendList().clear();

	for(auto& chan : GlobalChannelList())
	{
		chan.backends.clear();
	}
}

void Log::DisableBackendForChannel(LogChannel chan, LogBackend backend)
{
	if(!CheckChannelId(chan) || !CheckBackendId(backend))
		return;

	auto lock = GlobalLogMutex()->RAIILock();

	auto desc = GetChanDesc(chan);
	desc->backends.remove(backend);
}

void Log::EnableBackendForChannel(LogChannel chan, LogBackend backend)
{
	if(!CheckChannelId(chan) || !CheckBackendId(backend))
		return;

	auto lock = GlobalLogMutex()->RAIILock();

	auto desc = GetChanDesc(chan);
	desc->backends.push_back(backend);
}

void Log::DisableBackend(LogBackend backend)
{
	if(!CheckBackendId(backend))
		return;

	auto lock = GlobalLogMutex()->RAIILock();

	for(auto& c : GlobalChannelList())
	{
		c.backends.remove(backend);
	}
}

void Log::EnableBackend(LogBackend backend)
{
	if(!CheckBackendId(backend))
		return;

	auto lock = GlobalLogMutex()->RAIILock();

	for(auto& c : GlobalChannelList())
	{
		c.backends.push_back(backend);
	}
}

static void LogInternal(LogChannel chan, LogChannelDescription_t* desc, ELogLevel level, LogColor color, const char* message)
{
	if(!desc)
		desc = GetChanDesc(chan);

	/* Invoke all backends */
	for(auto& backend : desc->backends)
	{
		ILogBackend* b = GlobalBackendList()[backend];

		if(!b)
			continue;

		b->Log(chan, level, color, message);
	}
}

void Log::Log(LogChannel chan, ELogLevel level, LogColor color, const char* fmt, ...)
{
	if(!CheckChannelId(chan) || !fmt)
		return;

	thread_local static char buf[4096];
	va_list list;
	va_start(list, fmt);
	vsnprintf(buf, sizeof(buf), fmt, list);
	va_end(list);

	auto lock = GlobalLogMutex()->RAIILock();

	LogInternal(chan, nullptr, level, color, buf);
}

void Log::Log(LogChannel chan, ELogLevel level, const char* fmt, ...)
{
	if(!CheckChannelId(chan) || !fmt)
		return;

	thread_local static char buf[4096];
	va_list list;
	va_start(list, fmt);
	vsnprintf(buf, sizeof(buf), fmt, list);
	va_end(list);

	auto lock = GlobalLogMutex()->RAIILock();

	auto desc = GetChanDesc(chan);
	if(!desc)
		return;

	LogInternal(chan, desc, level, desc->defaultColor, buf);
}

void Log::Log(LogChannel chan, ELogLevel level, LogColor color, const char* fmt, va_list list)
{
	if(!CheckChannelId(chan) || !fmt)
		return;

	thread_local static char buf[4096];
	vsnprintf(buf, sizeof(buf), fmt, list);

	auto lock = GlobalLogMutex()->RAIILock();

	LogInternal(chan, nullptr, level, color, buf);
}

void Log::Log(LogChannel chan, ELogLevel level, const char* fmt, va_list list)
{
	if(!CheckChannelId(chan) || !fmt)
		return;

	thread_local static char buf[4096];
	vsnprintf(buf, sizeof(buf), fmt, list);

	auto lock = GlobalLogMutex()->RAIILock();

	auto desc = GetChanDesc(chan);
	if(!desc)
		return;

	LogInternal(chan, desc, level, desc->defaultColor, buf);
}

LogGroup Log::CreateGroup(const char* name)
{
	auto lock = GlobalLogMutex()->RAIILock();

	return 0;
}


void Log::AddChannelToGroup(LogChannel chan, LogGroup group)
{
	if(!CheckChannelId(chan) || !CheckGroupId(group))
		return;

	auto lock = GlobalLogMutex()->RAIILock();

	auto desc = GetChanDesc(chan);
	desc->groups.push_back(group);
}

void Log::RemoveChannelFromGroup(LogChannel chan, LogGroup group)
{
	if(!CheckChannelId(chan) || !CheckGroupId(group))
		return;

	auto lock = GlobalLogMutex()->RAIILock();

	auto desc = GetChanDesc(chan);
	desc->groups.remove(group);
}

bool Log::IsChannelInGroup(LogChannel chan, LogGroup group)
{
	if(!CheckChannelId(chan) || !CheckGroupId(group))
		return false;

	auto lock = GlobalLogMutex()->RAIILock();

	auto desc = GetChanDesc(chan);

	return desc->groups.contains(group);
}

int Log::NumChannelsInGroup(LogGroup grp)
{
	if(!CheckGroupId(grp))
		return 0;
	return 0;
}

//===========================================================================//
//
// DEFAULT LOG BACKEND
//
//===========================================================================//

Log::DefaultLogBackend::DefaultLogBackend(bool colorized)
{
#ifdef _POSIX
	m_colorized = false;
	char* e = getenv("COLORTERMINAL");
	if(e && strcmp(e, "truecolor") == 0)
	{
		m_colorized = colorized;
	}
	e = getenv("TERM");
	if(e && strcmp(e, "xterm-256color") == 0)
	{
		m_colorized = colorized;
	}
#endif
}

void Log::DefaultLogBackend::Log(LogChannel chan, ELogLevel lvl, LogColor color, const char* msg)
{
	this->SetStreamColor(color.r, color.g, color.b);
	if(m_colorized && (lvl == ELogLevel::MSG_ERROR || lvl == ELogLevel::MSG_FATAL))
		SetStreamBold();
	if(m_colorized)
		Q_fmtcolorstr_stream(stdout, msg);
	else
		fputs(msg, stdout);
	this->ResetStreamColor();
}

void Log::DefaultLogBackend::SetStreamColor(unsigned char r, unsigned char g, unsigned char b)
{
#ifdef _POSIX
	if(m_colorized)
	{
		fprintf(stderr, "\x1b[38;2;%u;%u;%um", r, g, b);
		fprintf(stdout, "\x1b[38;2;%u;%u;%um", r, g, b);
	}
#endif
}

void Log::DefaultLogBackend::SetStreamBold()
{
#ifdef _POSIX
	if(m_colorized)
	{
		fprintf(stderr, "\x1b[1m\n");
		fprintf(stdout, "\x1b[1m\n");
	}
#endif
}

void Log::DefaultLogBackend::ResetStreamColor()
{
#ifdef _POSIX
	if(m_colorized)
	{
		fprintf(stdout, "\x1b[0m");
		fprintf(stderr, "\x1b[0m");
	}
#endif
}

//===========================================================================//
//
// FILEIO STDIO LOG BACKEND
//
//===========================================================================//

Log::StdFileLogBackend::StdFileLogBackend(const char* path)
{
	m_stream = fopen(path, "w");
	m_path = path;
}

Log::StdFileLogBackend::~StdFileLogBackend()
{
	if(m_stream)
		fclose(m_stream);
}

void Log::StdFileLogBackend::Log(LogChannel chan, ELogLevel lvl, LogColor color, const char* msg)
{
	if(!m_stream)
		return;
	fputs(msg, m_stream);
}
