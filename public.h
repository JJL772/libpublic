#pragma once

#ifdef PUBLIC_STANDALONE
using byte = unsigned char;
using qboolean = int;
using uint = unsigned int;
using dword = unsigned int;
typedef char string[256];
using vec_t = float;

#else
#include "xash3d_types.h"
#include "port.h"
#endif 

#ifdef PUBLIC_EXPORT
	#ifdef _WIN32
		#ifdef _MSC_VER
			#define PUBLICAPI __declspec(dllexport)
		#else
			#define PUBLICAPI __attribute__((dllexport))
		#endif
	#else
		#define PUBLICAPI __attribute__((visibility ("default")))
	#endif
#else
	#ifdef _WIN32
		#ifdef _MSC_VER
			#define PUBLICAPI __declspec(dllimport)
		#else
			#define PUBLICAPI __attribute__((dllimport))
		#endif
	#else
		#define PUBLICAPI
	#endif
#endif


#ifdef PUBLIC_STANDALONE

#define Q_min(a,b) (((a)<(b))?(a):(b))
#define Q_max(a,b) (((a)>(b))?(a):(b))

static bool IsColorString(const char* p) { return (( p && *( p ) == '^' && *(( p ) + 1) && *(( p ) + 1) >= '0' && *(( p ) + 1 ) <= '9' )); };
static bool ColorIndex(char c) { return (c - '0') & 7; };

#define MAX_OSPATH 4096
#define MAX_PATH 260

#ifdef PUBLIC_EXPORT
	#ifdef _WIN32
		#ifdef _MSC_VER
			#define EXPORT __declspec(dllexport)
		#else
			#define EXPORT __attribute__((dllexport))
		#endif
	#else
		#define EXPORT __attribute__((visibility ("default")))
	#endif
#else
	#ifdef _WIN32
		#ifdef _MSC_VER
			#define EXPORT __declspec(dllimport)
		#else
			#define EXPORT __attribute__((dllimport))
		#endif
	#else
		#define EXPORT
	#endif
#endif
#ifdef _WIN32
        #ifdef _MSC_VER
                #define IMPORT __declspec(dllimport)
        #else
                #define IMPORT __attribute__((dllimport)) 
        #endif
#else
        #define IMPORT 
#endif 
#endif // PUBLIC_STANDALINE
