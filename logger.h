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
#pragma once

#include "public.h"

#define COLOR_NORMAL "^1"
#define COLOR_RED "^2"
#define COLOR_GREEN "^3"
#define COLOR_BLUE "^4"
#define COLOR_PURPLE "^5"
#define COLOR_YELLOW "^6"
#define COLOR_BOLD "^7"
#define COLOR_STRIKE "^8"

namespace logger
{
	EXPORT void Printf(const char* fmt, ...);
	EXPORT void Errorf(const char* fmt, ...);
	EXPORT void Warnf(const char* fmt, ...);
}
