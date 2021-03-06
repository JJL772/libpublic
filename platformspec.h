/**
 *
 * platformspec.h - Platform specific code
 *
 */
#pragma once

#include "public.h"

#undef GetCurrentTime

namespace platform
{
/**
 * Format of the time_t structure:
 * 	sec corresponds to the number of seconds since UNIX epoch
 * 	ns corresponds to the number of nano-seconds within the current second. Meaning, ns will always be less than
 * 		1e9, as once it reaches 1e9 it will be reset and sec will be incremented
 */
struct EXPORT time_t
{
	unsigned long long sec;
	unsigned long long ns;

	unsigned long long to_ns() const { return ns + (sec * 1000000000); };
	double		   to_seconds() const { return sec + (ns / 1e9); };
	double		   to_ms() const { return (sec * 1e3) + (ns / 1e6); };

	bool operator<(const time_t&) const;
	bool operator<=(const time_t&) const;
	bool operator>(const time_t&) const;
	bool operator>=(const time_t&) const;
	bool operator==(const time_t&) const;
	bool operator!=(const time_t&) const;
};

EXPORT time_t GetCurrentTime();

EXPORT unsigned long long GetCurrentThreadId();

/* Fatal error function */
EXPORT void FatalError(const char* fmt, ...);

/* Execute a command */
EXPORT int ExecProgram(const char* prog, char* const args[], const char* env[] = nullptr);

} // namespace platform
