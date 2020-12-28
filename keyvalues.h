/*
keyvalues.cpp - Keyvalues parser
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

#undef min
#undef max
#include <vector>
#include <stdio.h>

#include "public.h"

class EXPORT KeyValues
{
public:
	struct key_t
	{
	private:
		char* key;
		char* value;
		enum class ELastCached
		{
			NONE = 0,
			INT,
			FLOAT,
			BOOL,
		} cached;

		union
		{
			long int ival;
			double	 fval;
			bool	 bval;
		} cachedv;

		bool quoted;
		friend class KeyValues;

	public:
		key_t() { cached = ELastCached::NONE; };

		inline bool	ReadBool(bool& ok);
		inline long int ReadInt(bool& ok);
		inline double	ReadFloat(bool& ok);

		const char* Name() const { return key; };
		const char* Value() const { return value; };
		bool	    Quoted() const { return quoted; };
	};

private:
	char* name;
	bool  good;
	bool  quoted;

public:
	KeyValues(const KeyValues& kv);
	KeyValues(KeyValues&& kv);
	explicit KeyValues(const char* name);
	KeyValues();

	~KeyValues();

	KeyValues& operator=(const KeyValues& kv);
	KeyValues& operator=(KeyValues&& kv);

	/* Getters */
	bool	    GetBool(const char* key, bool _default = false);
	int	    GetInt(const char* key, int _default = -1);
	float	    GetFloat(const char* key, float _default = 0.0f);
	const char* GetString(const char* key, const char* _default = nullptr);
	double	    GetDouble(const char* key, double _default = 0.0);
	KeyValues*  GetChild(const char* name);

	bool HasKey(const char* key);

	/* Setters */
	void SetBool(const char* key, bool v);
	void SetInt(const char* key, int v);
	void SetFloat(const char* key, float v);
	void SetString(const char* key, const char* v);

	/* Parse from a file */
	void ParseFile(const char* file, bool use_escape_codes = false);
	void ParseFile(FILE* f, bool use_escape_codes = false);
	void ParseString(const char* string, bool use_escape_codes = false, long long len = -1);

	const std::vector<key_t>& Keys() const { return keys; };
	const char*		  Name() const { return name; };
	bool			  Quoted() const { return quoted; };

	/* Clears a key's value setting it to "" */
	void ClearKey(const char* key);

	/* Completely removes a key */
	void RemoveKey(const char* key);

	/* Dumps this to stdout */
	void DumpToStream(FILE* fs);
	void DumpToStreamInternal(FILE* fs, int indent);

	/* Set Good bit is set if parsing went OK */
	bool IsGood() const { return this->good; };

	enum class EError
	{
		NONE = 0,
		UNEXPECTED_EOF,
		MISSING_BRACKET,
		MISSING_QUOTE,
		UNNAMED_SECTION,
		UNTERMINATED_SECTION,
	};

	typedef void (*pfnErrorCallback_t)(int, int, EError);

	/* Sets the error reporting callback */
	void SetErrorCallback(pfnErrorCallback_t callback);

	/* Array of child sections */
	std::vector<KeyValues*> child_sections;

	/* Array of keys */
	std::vector<key_t> keys;

private:
	void		   ReportError(int line, int _char, EError err);
	pfnErrorCallback_t pCallback;
};
