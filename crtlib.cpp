/*
crtlib.c - internal stdlib
Copyright (C) 2011 Uncle Mike
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
#ifndef PUBLIC_STANDALONE
#include "port.h"
#include "const.h"
#endif
#include "public.h"
#include <math.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include "stdio.h"
#include "crtlib.h"
#include "mem.h"

#ifdef OS_POSIX
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <process.h>
#include <io.h>
#include <malloc.h>
#undef min
#undef max
#else
#include <sys/stat.h>
#endif

#define BIT0 (1)
#define BIT1 (2)
#define BIT2 (4)
#define BIT3 (8)
#define BIT4 (16)
#define BIT5 (32)
#define BIT6 (64)
#define BIT7 (128)

byte* g_pCrtPool;

#ifdef _WIN32
#define PATH_SEPARATORC '\\'
#else
#define PATH_SEPARATORC '/'
#endif

static void InitCrtLib()
{
	static bool binit = false;
	if (binit)
		return;
	g_pCrtPool = GlobalAllocator()._Mem_AllocPool("CrtlibPool", __FILE__, __LINE__);
}

int Q_countchar(const char* str, char c)
{
	int i = 0;
	for (; *str; str++)
		if (str[i] == c)
			i++;
	return i;
}

bool Q_startswith(const char* str, const char* sub) { return Q_strncmp(str, sub, strlen(sub)) == 0; }

bool Q_endswith(const char* str, const char* subst)
{
	if (!str || !subst)
		return false;
	size_t sz   = strlen(subst);
	size_t sztr = strlen(str);
	if (sz > sztr)
		return false;
	return Q_strncmp(&str[sztr - sz], subst, sz) == 0;
}

void Q_strnupr(const char* in, char* out, size_t size_out)
{
	if (size_out == 0)
		return;

	while (*in && size_out > 1)
	{
		if (*in >= 'a' && *in <= 'z')
			*out++ = *in++ + 'A' - 'a';
		else
			*out++ = *in++;
		size_out--;
	}
	*out = '\0';
}

void Q_strnlwr(const char* in, char* out, size_t size_out)
{
	if (size_out == 0)
		return;

	while (*in && size_out > 1)
	{
		if (*in >= 'A' && *in <= 'Z')
			*out++ = *in++ + 'a' - 'A';
		else
			*out++ = *in++;
		size_out--;
	}
	*out = '\0';
}

qboolean Q_isdigit(const char* str)
{
	if (str && *str)
	{
		while (isdigit(*str))
			str++;
		if (!*str)
			return true;
	}
	return false;
}

int Q_strlen(const char* string)
{
	if (!string)
		return 0;
	return strlen(string);
}

int Q_colorstr(const char* string)
{
	int	    len;
	const char* p;

	if (!string)
		return 0;

	len = 0;
	p   = string;
	while (*p)
	{
		if (IsColorString(p))
		{
			len += 2;
			p += 2;
			continue;
		}
		p++;
	}

	return len;
}

static unsigned char g_default_color_table[][3] = {
	{255, 255, 255}, // 0 (white)
	{255, 0, 0},	 // 1 (red)
	{0, 255, 0},	 // 2 (green)
	{255, 255, 0},	 // 3 (yellow)
	{0, 255, 255},	 // 4 (cyan)
	{255, 0, 255},	 // 5 (purple)
	{255, 155, 0},	 // 6 (orange)
	{255, 255, 255}, // 7 (white)
	{255, 100, 100}, // 8 (light red)
	{100, 255, 100}, // 9 (light green)
};

static unsigned char g_default_format_modifiers[] = {
	FMT_NONE, // 0
	FMT_BOLD, // 1
	FMT_NONE, // 2
	FMT_BOLD, // 3
	FMT_NONE, // 4
	FMT_NONE, // 5
	FMT_NONE, // 6
	FMT_BOLD, // 7
	FMT_NONE, // 8
	FMT_NONE, // 9
};

char* Q_fmtcolorstr(const char* s, char* out, size_t n) { return Q_fmtcolorstr(s, out, n, g_default_color_table, g_default_format_modifiers); }

char* Q_fmtcolorstr(const char* s, char* out, size_t n, const unsigned char color_table[10][3])
{
	return Q_fmtcolorstr(s, out, n, color_table, g_default_format_modifiers);
}

// TODO: PROFILE THIS!!
char* Q_fmtcolorstr(const char* s, char* out, size_t sz, const unsigned char color_table[10][3], const unsigned char format_modifiers[10])
{
	if (!s || !out)
		return nullptr;

	struct color_str_ptr
	{
		const char* ptr;
		size_t	    len;
		char	    color_index;
	};

	/* Allocate and clear string list */
	int color_strs	    = Q_colorstr(s) / 2; // NOTE: colorstr returns counts in muliples of 2, for the char count of the actual "color string"
	color_str_ptr* ptrs = (color_str_ptr*)Q_alloca(sizeof(color_str_ptr) * color_strs);
	memset(ptrs, 0, sizeof(color_str_ptr) * color_strs);

	int    cur_str = 0;
	size_t nlen    = Q_strlen(s);
	size_t prevlen = 0;

	for (size_t i = 0; i < nlen; i++)
	{
		if (s[i] == '^')
		{
			/* If the current pointer is set, that means we've already come through this routine once. */
			/* So compute the length and increment the current string */
			if (ptrs[cur_str].ptr)
			{
				ptrs[cur_str].len = i - prevlen;
				cur_str++;
			}

			i++;
			if (i >= nlen + 1)
				break;
			if (s[i] > '9' || s[i] < '0')
				continue;
			ptrs[cur_str].color_index = (char)(s[i] - 0x30); // Convert to char for the actual index
			ptrs[cur_str].ptr	  = &s[i + 1];		 // set the pointer to the start of the substring
			prevlen			  = i + 1;		 // set the previous index
			continue;
		}
	}
	/* Set the last length to nlen - prevlen */
	ptrs[cur_str].len = nlen - prevlen;
	out[0]		  = 0;

	/* Index is the format specifier (FMT_XXX) */
	static int format_conversion_table[] = {
		0, // FMT_NONE
		1, // FMT_BOLD
		4, // FMT_UNDERLINE
		5, // FMT_BLINK
	};

	/* Loop through our list of strings and concatenate them, including coloring info */
	int total_printed = 0;
	for (int i = 0; i < color_strs; i++)
	{
		if (!ptrs[i].ptr)
			continue;

		/* Windows 10 anniversary edition supports ANSI escape codes for colors in CMD */
		/* Print in the color string */
		const unsigned char* color = color_table[(int)ptrs[i].color_index];
		unsigned char	     mod   = format_conversion_table[format_modifiers[(int)ptrs[i].color_index]];
		char		     fmtstr[32];
		if (mod != 0)
			total_printed += snprintf(fmtstr, sizeof(fmtstr), "\x1b[38;2;%u;%u;%um\x1b[%um", (unsigned char)color[0],
						  (unsigned char)color[1], (unsigned char)color[2], mod);
		else
			total_printed += snprintf(fmtstr, sizeof(fmtstr), "\x1b[38;2;%u;%u;%um", (unsigned char)color[0], (unsigned char)color[1],
						  (unsigned char)color[2]);
		strncat(out, fmtstr, sz);

		/* Concat the rest of it */
		/* NOTE: Using memcpy here because the string is not null terminated */
		if (ptrs[i].len + total_printed > sz)
		{
			/* Dont overflow the buffer and break out of the loop as we can't print any more */
			memcpy(&out[total_printed], ptrs[i].ptr, sz - ptrs[i].len);
			out[sz] = 0;
			break;
		}
		else
		{
			memcpy(&out[total_printed], ptrs[i].ptr, ptrs[i].len);
			out[total_printed + ptrs[i].len] = 0;
		}
		total_printed += ptrs[i].len;
	}
	return out;
}

void Q_fmtcolorstr_stream(FILE* stream, const char* s) { Q_fmtcolorstr_stream(stream, s, g_default_color_table, g_default_format_modifiers); }

void Q_fmtcolorstr_stream(FILE* stream, const char* s, const unsigned char color_table[10][3])
{
	Q_fmtcolorstr_stream(stream, s, color_table, g_default_format_modifiers);
}

void Q_fmtcolorstr_stream(FILE* stream, const char* s, const unsigned char color_table[10][3], const unsigned char format_modifiers[10])
{
	if (!s)
		return;

	struct color_str_ptr
	{
		const char* ptr;
		size_t	    len;
		char	    color_index;
	};

	/* Allocate and clear string list */
	int color_strs	    = Q_colorstr(s) / 2; // NOTE: colorstr returns counts in muliples of 2, for the char count of the actual "color string"
	color_str_ptr* ptrs = (color_str_ptr*)Q_alloca(sizeof(color_str_ptr) * color_strs);
	memset(ptrs, 0, sizeof(color_str_ptr) * color_strs);

	int    cur_str = 0;
	size_t nlen    = Q_strlen(s);
	size_t prevlen = 0;

	for (size_t i = 0; i < nlen; i++)
	{
		if (s[i] == '^')
		{
			/* If the current pointer is set, that means we've already come through this routine once. */
			/* So compute the length and increment the current string */
			if (ptrs[cur_str].ptr)
			{
				ptrs[cur_str].len = i - prevlen;
				cur_str++;
			}

			i++;
			if (i >= nlen + 1)
				break;
			if (s[i] > '9' || s[i] < '0')
				continue;
			ptrs[cur_str].color_index = (char)(s[i] - 0x30); // Convert to char for the actual index
			ptrs[cur_str].ptr	  = &s[i + 1];		 // set the pointer to the start of the substring
			prevlen			  = i + 1;		 // set the previous index
			continue;
		}
	}
	/* Set the last length to nlen - prevlen */
	ptrs[cur_str].len = nlen - prevlen;

	/* Index is the format specifier (FMT_XXX) */
	static int format_conversion_table[] = {
		0, // FMT_NONE
		1, // FMT_BOLD
		4, // FMT_UNDERLINE
		5, // FMT_BLINK
	};

	/* if we have no color strs, just print verbatim */
	if (color_strs == 0)
	{
		fputs(s, stream);
		return;
	}

	/* Loop through our list of strings and concatenate them, including coloring info */
	for (int i = 0; i < color_strs; i++)
	{
		if (!ptrs[i].ptr)
			continue;

		/* Windows 10 anniversary edition supports ANSI escape codes for colors in CMD */
		/* Print in the color string */
		const unsigned char* color = color_table[(int)ptrs[i].color_index];
		unsigned char	     mod   = format_conversion_table[format_modifiers[(int)ptrs[i].color_index]];
		if (mod != 0)
			fprintf(stream, "\x1b[38;2;%u;%u;%um\x1b[%um", (unsigned char)color[0], (unsigned char)color[1], (unsigned char)color[2],
				mod);
		else
			fprintf(stream, "\x1b[38;2;%u;%u;%um", (unsigned char)color[0], (unsigned char)color[1], (unsigned char)color[2]);
		fwrite(ptrs[i].ptr, ptrs[i].len, 1, stream);
	}
	fputs("\x1b[0m", stream);
}

char Q_toupper(const char in)
{
	char out;

	if (in >= 'a' && in <= 'z')
		out = in + 'A' - 'a';
	else
		out = in;

	return out;
}

char Q_tolower(const char in)
{
	char out;

	if (in >= 'A' && in <= 'Z')
		out = in + 'a' - 'A';
	else
		out = in;

	return out;
}

size_t Q_strncat(char* dst, const char* src, size_t size)
{
	char*	    d = dst;
	const char* s = src;
	size_t	    n = size;
	size_t	    dlen;

	if (!dst || !src || !size)
		return 0;

	// find the end of dst and adjust bytes left but don't go past end
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n    = size - dlen;

	if (n == 0)
		return (dlen + Q_strlen(s));

	while (*s != '\0')
	{
		if (n != 1)
		{
			*d++ = *s;
			n--;
		}
		s++;
	}

	*d = '\0';
	return (dlen + (s - src)); // count does not include NULL
}

size_t Q_strncpy(char* dst, const char* src, size_t size)
{
	char*	    d = dst;
	const char* s = src;
	size_t	    n = size;

	if (!dst || !src || !size)
		return 0;

	// copy as many bytes as will fit
	if (n != 0 && --n != 0)
	{
		do
		{
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	// not enough room in dst, add NULL and traverse rest of src
	if (n == 0)
	{
		if (size != 0)
			*d = '\0'; // NULL-terminate dst
		while (*s++)
			;
	}
	return (s - src - 1); // count does not include NULL
}

int Q_atoi(const char* str)
{
	int val = 0;
	int c, sign;

	if (!str)
		return 0;

	// check for empty charachters in string
	while (str && *str == ' ')
		str++;

	if (!str)
		return 0;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	// check for hex
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val << 4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val << 4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val << 4) + c - 'A' + 10;
			else
				return val * sign;
		}
	}

	// check for character
	if (str[0] == '\'')
		return sign * str[1];

	// assume decimal
	while (1)
	{
		c = *str++;
		if (c < '0' || c > '9')
			return val * sign;
		val = val * 10 + c - '0';
	}
	return 0;
}

float Q_atof(const char* str)
{
	double val = 0;
	int    c, sign, decimal, total;

	if (!str)
		return 0.0f;

	// check for empty charachters in string
	while (str && *str == ' ')
		str++;

	if (!str)
		return 0.0f;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	// check for hex
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val * 16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val * 16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val * 16) + c - 'A' + 10;
			else
				return val * sign;
		}
	}

	// check for character
	if (str[0] == '\'')
		return sign * str[1];

	// assume decimal
	decimal = -1;
	total	= 0;

	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}

		if (c < '0' || c > '9')
			break;
		val = val * 10 + c - '0';
		total++;
	}

	if (decimal == -1)
		return val * sign;

	while (total > decimal)
	{
		val /= 10;
		total--;
	}

	return val * sign;
}

void Q_atov(float* vec, const char* str, size_t siz)
{
	string buffer;
	char * pstr, *pfront;
	size_t    j;

	Q_strncpy(buffer, str, sizeof(buffer));
	memset(vec, 0, sizeof(vec_t) * siz);
	pstr = pfront = buffer;

	for (j = 0; j < siz; j++)
	{
		vec[j] = Q_atof(pfront);

		// valid separator is space
		while (*pstr && *pstr != ' ')
			pstr++;

		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}
}

char* Q_strchr(const char* s, char c)
{
	int len = Q_strlen(s);

	while (len--)
	{
		if (*++s == c)
			return (char*)s;
	}
	return 0;
}

char* Q_strrchr(const char* s, char c)
{
	int len = Q_strlen(s);

	s += len;

	while (len--)
	{
		if (*--s == c)
			return (char*)s;
	}
	return 0;
}

int Q_strnicmp(const char* s1, const char* s2, int n)
{
	int c1, c2;

	if (s1 == NULL)
	{
		if (s2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (s2 == NULL)
	{
		return 1;
	}

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0; // strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	// strings are equal
	return 0;
}

int Q_strncmp(const char* s1, const char* s2, int n)
{
	int c1, c2;

	if (s1 == NULL)
	{
		if (s2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (s2 == NULL)
	{
		return 1;
	}

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		// strings are equal until end point
		if (!n--)
			return 0;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;

	} while (c1);

	// strings are equal
	return 0;
}

int Q_strcasecmp(const char* s1, const char* s2)
{
	int c1, c2;

	if (s1 == NULL)
	{
		if (s2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (s2 == NULL)
	{
		return 1;
	}

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		/* Quick and dirty lil trick. Clearing bit 5 will ensure we have lowercase */
		// if ((c1 ^ BIT5) != (c2 ^ BIT5)) return c1 < c2 ? -1 : 1;
		if (Q_tolower(c1) != Q_tolower(c2))
			return c1 < c2 ? -1 : 1;

	} while (c1);

	// strings are equal
	return 0;
}

static qboolean Q_starcmp(const char* pattern, const char* text)
{
	char	    c, c1;
	const char *p = pattern, *t = text;

	while ((c = *p++) == '?' || c == '*')
	{
		if (c == '?' && *t++ == '\0')
			return false;
	}

	if (c == '\0')
		return true;

	for (c1 = ((c == '\\') ? *p : c);;)
	{
		if (Q_tolower(*t) == c1 && Q_stricmpext(p - 1, t))
			return true;
		if (*t++ == '\0')
			return false;
	}
}

qboolean Q_stricmpext(const char* pattern, const char* text)
{
	char c;

	while ((c = *pattern++) != '\0')
	{
		switch (c)
		{
		case '?':
			if (*text++ == '\0')
				return false;
			break;
		case '\\':
			if (Q_tolower(*pattern++) != Q_tolower(*text++))
				return false;
			break;
		case '*':
			return Q_starcmp(pattern, text);
		default:
			if (Q_tolower(c) != Q_tolower(*text++))
				return false;
		}
	}
	return (*text == '\0');
}

const char* Q_timestamp(int format)
{
	static string	 timestamp;
	time_t		 crt_time;
	const struct tm* crt_tm;
	string		 timestring;

	time(&crt_time);
	crt_tm = localtime(&crt_time);

	switch (format)
	{
	case TIME_FULL:
		// Build the full timestamp (ex: "Apr03 2007 [23:31.55]");
		strftime(timestring, sizeof(timestring), "%b%d %Y [%H:%M.%S]", crt_tm);
		break;
	case TIME_DATE_ONLY:
		// Build the date stamp only (ex: "Apr03 2007");
		strftime(timestring, sizeof(timestring), "%b%d %Y", crt_tm);
		break;
	case TIME_TIME_ONLY:
		// Build the time stamp only (ex: "23:31.55");
		strftime(timestring, sizeof(timestring), "%H:%M.%S", crt_tm);
		break;
	case TIME_NO_SECONDS:
		// Build the time stamp exclude seconds (ex: "13:46");
		strftime(timestring, sizeof(timestring), "%H:%M", crt_tm);
		break;
	case TIME_YEAR_ONLY:
		// Build the date stamp year only (ex: "2006");
		strftime(timestring, sizeof(timestring), "%Y", crt_tm);
		break;
	case TIME_FILENAME:
		// Build a timestamp that can use for filename (ex: "Nov2006-26 (19.14.28)");
		strftime(timestring, sizeof(timestring), "%b%Y-%d_%H.%M.%S", crt_tm);
		break;
	default:
		return NULL;
	}

	Q_strncpy(timestamp, timestring, sizeof(timestamp));

	return timestamp;
}

char* Q_strstr(const char* string, const char* string2)
{
	int c, len;

	if (!string || !string2)
		return NULL;

	c   = *string2;
	len = Q_strlen(string2);

	while (string)
	{
		for (; *string && *string != c; string++)
			;

		if (*string)
		{
			if (!Q_strncmp(string, string2, len))
				break;
			string++;
		}
		else
			return NULL;
	}
	return (char*)string;
}

char* Q_stristr(const char* string, const char* string2)
{
	int c, len;

	if (!string || !string2)
		return NULL;

	c   = Q_tolower(*string2);
	len = Q_strlen(string2);

	while (string)
	{
		for (; *string && Q_tolower(*string) != c; string++)
			;

		if (*string)
		{
			if (!Q_strnicmp(string, string2, len))
				break;
			string++;
		}
		else
			return NULL;
	}
	return (char*)string;
}

int Q_vsnprintf(char* buffer, size_t buffersize, const char* format, va_list args)
{
	size_t result;

#ifndef _MSC_VER
	result = vsnprintf(buffer, buffersize, format, args);
#else
	__try
	{
		result = _vsnprintf(buffer, buffersize, format, args);
	}

	// to prevent crash while output
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Q_strncpy(buffer, "^1sprintf throw exception^7\n", buffersize);
		result = buffersize;
	}
#endif

	if (result < 0 || result >= buffersize)
	{
		buffer[buffersize - 1] = '\0';
		return -1;
	}

	return result;
}

int Q_snprintf(char* buffer, size_t buffersize, const char* format, ...)
{
	va_list args;
	int	result;

	va_start(args, format);
	result = Q_vsnprintf(buffer, buffersize, format, args);
	va_end(args);

	return result;
}

int Q_sprintf(char* buffer, const char* format, ...)
{
	va_list args;
	int	result;

	va_start(args, format);
	result = Q_vsnprintf(buffer, 99999, format, args);
	va_end(args);

	return result;
}

uint Q_hashkey(const char* string, uint hashSize, qboolean caseinsensitive)
{
	uint i, hashKey = 0;

	if (caseinsensitive)
	{
		for (i = 0; string[i]; i++)
			hashKey += (i * 119) * Q_tolower(string[i]);
	}
	else
	{
		for (i = 0; string[i]; i++)
			hashKey += (i + 119) * (int)string[i];
	}

	hashKey = ((hashKey ^ (hashKey >> 10)) ^ (hashKey >> 20)) & (hashSize - 1);

	return hashKey;
}

char* Q_pretifymem(float value, int digitsafterdecimal)
{
	static char output[8][32];
	static int  current;
	float	    onekb = 1024.0f;
	float	    onemb = onekb * onekb;
	char	    suffix[8];
	char*	    out = output[current];
	char	    val[32], *i, *o, *dot;
	int	    pos;

	current = (current + 1) & (8 - 1);

	// first figure out which bin to use
	if (value > onemb)
	{
		value /= onemb;
		Q_sprintf(suffix, " Mb");
	}
	else if (value > onekb)
	{
		value /= onekb;
		Q_sprintf(suffix, " Kb");
	}
	else
		Q_sprintf(suffix, " bytes");

	// clamp to >= 0
	digitsafterdecimal = Q_max(digitsafterdecimal, 0);

	// if it's basically integral, don't do any decimals
	if (fabs(value - (int)value) < 0.00001)
	{
		Q_sprintf(val, "%i%s", (int)value, suffix);
	}
	else
	{
		char fmt[32];

		// otherwise, create a format string for the decimals
		Q_sprintf(fmt, "%%.%if%s", digitsafterdecimal, suffix);
		Q_sprintf(val, fmt, value);
	}

	// copy from in to out
	i = val;
	o = out;

	// search for decimal or if it was integral, find the space after the raw number
	dot = Q_strstr(i, ".");
	if (!dot)
		dot = Q_strstr(i, " ");

	pos = dot - i; // compute position of dot
	pos -= 3;      // don't put a comma if it's <= 3 long

	while (*i)
	{
		// if pos is still valid then insert a comma every third digit, except if we would be
		// putting one in the first spot
		if (pos >= 0 && !(pos % 3))
		{
			// never in first spot
			if (o != out)
				*o++ = ',';
		}

		pos--;	     // count down comma position
		*o++ = *i++; // copy rest of data as normal
	}
	*o = 0; // terminate

	return out;
}

/*
============
va

does a varargs printf into a temp buffer,
so I don't need to have varargs versions
of all text functions.
============
*/
char* va(const char* format, ...)
{
	va_list	    argptr;
	static char string[256][1024], *s;
	static int  stringindex = 0;

	s	    = string[stringindex];
	stringindex = (stringindex + 1) & 255;
	va_start(argptr, format);
	Q_vsnprintf(s, sizeof(string[0]), format, argptr);
	va_end(argptr);

	return s;
}

/*
============
COM_FileBase

Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
============
*/
void COM_FileBase(const char* in, char* out)
{
	int len, start, end;

	len = Q_strlen(in);
	if (!len)
		return;

	// scan backward for '.'
	end = len - 1;

	while (end && in[end] != '.' && in[end] != '/' && in[end] != '\\')
		end--;

	if (in[end] != '.')
		end = len - 1; // no '.', copy to end
	else
		end--; // found ',', copy to left of '.'

	// scan backward for '/'
	start = len - 1;

	while (start >= 0 && in[start] != '/' && in[start] != '\\')
		start--;

	if (start < 0 || (in[start] != '/' && in[start] != '\\'))
		start = 0;
	else
		start++;

	// length of new sting
	len = end - start + 1;

	// Copy partial string
	Q_strncpy(out, &in[start], len + 1);
	out[len] = 0;
}

/*
============
COM_FileExtension
============
*/
const char* COM_FileExtension(const char* in)
{
	const char *separator, *backslash, *colon, *dot;

	separator = Q_strrchr(in, '/');
	backslash = Q_strrchr(in, '\\');

	if (!separator || separator < backslash)
		separator = backslash;

	colon = Q_strrchr(in, ':');

	if (!separator || separator < colon)
		separator = colon;

	dot = Q_strrchr(in, '.');

	if (dot == NULL || (separator && (dot < separator)))
		return "";

	return dot + 1;
}

/*
============
COM_FileWithoutPath
============
*/
const char* COM_FileWithoutPath(const char* in)
{
	const char *separator, *backslash, *colon;

	separator = Q_strrchr(in, '/');
	backslash = Q_strrchr(in, '\\');

	if (!separator || separator < backslash)
		separator = backslash;

	colon = Q_strrchr(in, ':');

	if (!separator || separator < colon)
		separator = colon;

	return separator ? separator + 1 : in;
}

/*
============
COM_ExtractFilePath
============
*/
void COM_ExtractFilePath(const char* path, char* dest)
{
	const char* src = path + Q_strlen(path) - 1;

	// back up until a \ or the start
	while (src != path && !(*(src - 1) == '\\' || *(src - 1) == '/'))
		src--;

	if (src != path)
	{
		memcpy(dest, path, src - path);
		dest[src - path - 1] = 0; // cutoff backslash
	}
	else
		Q_strcpy(dest, ""); // file without path
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension(char* path)
{
	size_t length;

	length = Q_strlen(path) - 1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/' || path[length] == '\\' || path[length] == ':')
			return; // no extension
	}

	if (length)
		path[length] = 0;
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension(char* path, const char* extension)
{
	const char* src;

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	src = path + Q_strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		// it has an extension
		if (*src == '.')
			return;
		src--;
	}

	Q_strcat(path, extension);
}

/*
==================
COM_ReplaceExtension
==================
*/
void COM_ReplaceExtension(char* path, const char* extension)
{
	COM_StripExtension(path);
	COM_DefaultExtension(path, extension);
}

int matchpattern(const char* in, const char* pattern, qboolean caseinsensitive)
{
	return matchpattern_with_separator(in, pattern, caseinsensitive, "/\\:", false);
}

// wildcard_least_one: if true * matches 1 or more characters
//                     if false * matches 0 or more characters
int matchpattern_with_separator(const char* in, const char* pattern, qboolean caseinsensitive, const char* separators, qboolean wildcard_least_one)
{
	int c1, c2;

	while (*pattern)
	{
		switch (*pattern)
		{
		case 0:
			return 1; // end of pattern
		case '?':	  // match any single character
			if (*in == 0 || Q_strchr(separators, *in))
				return 0; // no match
			in++;
			pattern++;
			break;
		case '*': // match anything until following string
			if (wildcard_least_one)
			{
				if (*in == 0 || Q_strchr(separators, *in))
					return 0; // no match
				in++;
			}
			pattern++;
			while (*in)
			{
				if (Q_strchr(separators, *in))
					break;
				// see if pattern matches at this offset
				if (matchpattern_with_separator(in, pattern, caseinsensitive, separators, wildcard_least_one))
					return 1;
				// nope, advance to next offset
				in++;
			}
			break;
		default:
			if (*in != *pattern)
			{
				if (!caseinsensitive)
					return 0; // no match
				c1 = *in;
				if (c1 >= 'A' && c1 <= 'Z')
					c1 += 'a' - 'A';
				c2 = *pattern;
				if (c2 >= 'A' && c2 <= 'Z')
					c2 += 'a' - 'A';
				if (c1 != c2)
					return 0; // no match
			}
			in++;
			pattern++;
			break;
		}
	}
	if (*in)
		return 0; // reached end of pattern but not end of input
	return 1;	  // success
}

char* Q_strdup(const char* s)
{
	return strdup(s);
}

void* Q_malloc(size_t sz)
{
	return malloc(sz);
}

void Q_free(void* ptr)
{
	free(ptr);
}

bool Q_strint(const char* str, int& out, int base)
{
	if (!str)
		return false;
	long o = strtol(str, NULL, base);
	if (o == 0 && errno != 0)
		return false;
	out = o;
	return true;
}

bool Q_strfloat(const char* str, float& out)
{
	if (!str)
		return false;
	float o = strtof(str, NULL);
	if (o == 0.0f && errno != 0)
		return false;
	out = o;
	return true;
}

bool Q_strlong(const char* str, long long int& out, int base)
{
	if (!str)
		return false;
	long long o = strtol(str, NULL, base);
	if (o == 0 && errno != 0)
		return false;
	out = o;
	return true;
}

bool Q_strdouble(const char* str, double& out)
{
	if (!str)
		return false;
	float o = strtod(str, NULL);
	if (o == 0.0 && errno != 0)
		return false;
	out = o;
	return true;
}

bool Q_strbool(const char* str, bool& out)
{
	if (!str)
		return false;
	out = !(Q_strcasecmp(str, "FALSE") == 0 || Q_strcasecmp(str, "0") == 0);
	return true;
}

char* Q_FileExtension(const char* s, char* out, size_t len)
{
	if (!s || !out || len == 0)
		return nullptr;
	const char* ext	 = COM_FileExtension(s);
	size_t	    _len = Q_strlen(ext);
	_len		 = _len > len ? len : _len;
	memcpy(out, ext, _len);
	if (_len != len)
		out[_len] = 0;
	else
		out[_len - 1] = 0;
	return out;
}

char* Q_FileName(const char* s, char* out, size_t len)
{
	if (!s || !out || len == 0)
		return nullptr;
	const char* file = COM_FileWithoutPath(s);
	size_t	    _len = Q_strlen(file);
	_len		 = _len > len ? len : _len;
	memcpy(out, file, _len);
	if (_len != len)
		out[_len] = 0;
	else
		out[_len - 1] = 0;
	return out;
}

char* Q_BaseDirectory(const char* path, char* dest, size_t len)
{
	const char* src = path + strlen(path) - 1;

	// back up until a \ or the start
	while (src != path && !(*(src - 1) == '\\' || *(src - 1) == '/'))
		src--;

	if (src != path)
	{
		size_t max = src - path;
		memcpy(dest, path, max > len ? len : max);
		dest[src - path - 1] = 0; // cutoff backslash
	}
	else
		strncpy(dest, "", len);
	return dest;
}

char* Q_StripExtension(const char* s, char* out, size_t len)
{
	if (!s || !out || len == 0)
		return nullptr;
	size_t _len = Q_strlen(s);
	_len	    = _len > len ? len : _len;
	memcpy(out, s, _len);
	COM_StripExtension(out);
	return out;
}

char* Q_StripDirectory(const char* s, char* out, size_t len)
{
	if (!s || !out || len == 0)
		return nullptr;
	const char* file = COM_FileWithoutPath(s);
	size_t	    _len = Q_strlen(s);
	_len		 = _len > len ? len : _len;
	memcpy(out, file, _len);
	if (_len != len)
		out[_len] = 0;
	else
		out[_len - 1] - 0;
	return out;
}

char* Q_FixSlashes(const char* s, char* out, size_t len)
{
	if (!s || !out || len == 0)
		return nullptr;
	size_t _len = Q_strlen(s);
	_len	    = _len > len ? len : _len;
	memcpy(out, s, _len);
	if (_len != len)
		out[_len] = 0;
	else
		out[_len - 1] = 0;
	for (size_t i = 0; i < _len; i++)
	{
		if (out[i] == '\\' || out[i] == '/')
			out[i] = PATH_SEPARATORC;
	}
	return out;
}

char* Q_FixSlashesInPlace(char* s)
{
	if (!s)
		return nullptr;
	size_t len = Q_strlen(s);
	for (size_t i = 0; i < len; i++)
	{
		if (s[i] == '\\' || s[i] == '/')
			s[i] = PATH_SEPARATORC;
	}
	return s;
}

String& Q_FixSlashesInPlace(String& s)
{
	char*  _s  = (char*)s;
	size_t len = Q_strlen(_s);
	for (size_t i = 0; i < len; i++)
	{
		if (_s[i] == '\\' || _s[i] == '/')
			_s[i] = PATH_SEPARATORC;
	}
	return s;
}

char* Q_MakeAbsolute(const char* s, char* out, size_t len)
{
	if (!s || !out || len == 0)
		return nullptr;
#ifdef _WIN32
	_fullpath(out, s, len);
	return out;
#else
	return realpath(s, out);
#endif
}

char* Q_getcwd(char* buf, size_t sz)
{
#ifdef _WIN32
	return _getcwd(buf, sz);
#else
	return getcwd(buf, sz);
#endif
}

int Q_getpid()
{
#ifdef _WIN32
	return _getpid();
#else
	return getpid();
#endif
}

int Q_mkstemp(char* tmpl)
{
#ifdef _WIN32
	return _mktemp(tmpl) ? 0 : -1;
#else
	return mkstemp(tmpl);
#endif
}

int Q_unlink(const char* path)
{
#ifdef _WIN32
	return _unlink(path);
#else
	return unlink(path);
#endif
}

int Q_mkdir(const char* path)
{
#ifdef _WIN32
	return _mkdir(path);
#else
	return mkdir(path, S_IRWXU | S_IRWXG);
#endif
}

int Q_fileno(FILE* f)
{
#ifdef _WIN32
	return _fileno(f);
#else
	return fileno(f);
#endif
}

char* Q_BuildLibraryName(const char* basename, char* buffer, size_t bufsize)
{
	Q_snprintf(buffer, bufsize, "%s%s%s", DLL_PREFIX, basename, DLL_EXT);
	return buffer;
}

int  Q_chdir(const char* path)
{
	return chdir(path);
}

char* Q_realpath(const char* path, char* resolved, size_t size_of_resolved)
{
#ifdef OS_POSIX
	return realpath(path, resolved);
#else
	return _fullpath(resolved, path, size_of_resolved);
#endif
}
