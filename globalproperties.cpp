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
#include "globalproperties.h"

bool gProperties[EGlobalProperty::PROPERTY_COUNT];

bool GetProperty(EGlobalProperty prop)
{
	return gProperties[prop];
}

void SetProperty(EGlobalProperty prop, bool value)
{
	gProperties[prop] = value;
}
