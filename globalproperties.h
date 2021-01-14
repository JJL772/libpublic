/*
globalproperty.cpp - Global properties
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

/* List of global properties that any binary should be able to query */
/* The idea here is to nuke the XASH_DEDICATED ifdefs, and the best way to do this is to */
/* provide a global property handler to use in stupid if statements */
/* All of the XASH_DEDICATED protected code builds outside of the switch- we're not dealing */
/* with platform-specific code as much as we're dealing with domain-specific *behaviour* */
/* At worst this will result in some branch mis-predictions, but branch prediction is pretty good these days */

enum EGlobalProperty
{
	PROPERTY_DEDICATED_SERVER,
	PROPERTY_PREFER_LOW_MEMORY,
	PROPERTY_RESERVED0,
	PROPERTY_RESERVED1,
	PROPERTY_LAST = PROPERTY_RESERVED1,
	PROPERTY_COUNT
};

bool GetGlobalProperty(EGlobalProperty prop);
void SetGlobalProperty(EGlobalProperty prop, bool value);
