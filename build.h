/*
build.h - compile-time build information
Copyright (C) 2019 a1batross
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
#ifndef BUILD_H
#define BUILD_H

// All XASH_* macros set by this header are guaranteed to have positive value otherwise not defined

#undef XASH_64BIT
#undef XASH_WIN32
#undef XASH_MINGW
#undef XASH_MSVC
#undef XASH_LINUX
#undef XASH_ANDROID
#undef XASH_APPLE
#undef XASH_IOS
#undef XASH_BSD
#undef XASH_FREEBSD
#undef XASH_NETBSD
#undef XASH_OPENBSD
#undef XASH_EMSCRIPTEN
#undef XASH_MOBILE_PLATFORM
#undef XASH_LITTLE_ENDIAN
#undef XASH_BIG_ENDIAN
#undef XASH_AMD64
#undef XASH_X86
#undef XASH_ARM64
#undef XASH_ARM
#undef XASH_ARM_SOFTFP
#undef XASH_ARM_HARDFP
#undef XASH_MIPS
#undef XASH_JS
#undef XASH_E2K

//================================================================
//
//           OPERATING SYSTEM DEFINES
//
//================================================================
#if defined(_WIN32)
#define XASH_WIN32 1
#if defined(__MINGW32__)
#define XASH_MINGW 1
#elif defined(_MSC_VER)
#define XASH_MSVC 1
#endif

#if defined(_WIN64)
#define XASH_WIN64 1
#endif
#elif defined(__linux__)
#define XASH_LINUX 1
#if defined(__ANDROID__)
#define XASH_ANDROID 1
#endif // defined(__ANDROID__)
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#define XASH_APPLE 1
#if TARGET_OS_IOS
#define XASH_IOS 1
#endif // TARGET_OS_IOS
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define XASH_BSD 1
#if defined(__FreeBSD__)
#define XASH_FREEBSD 1
#elif defined(__NetBSD__)
#define XASH_NETBSD 1
#elif defined(__OpenBSD__)
#define XASH_OPENBSD 1
#endif
#elif defined __EMSCRIPTEN__
#define XASH_EMSCRIPTEN 1
#else
#error "Place your operating system name here! If this is a mistake, try to fix conditions above and report a bug"
#endif

#if defined XASH_ANDROID || defined XASH_IOS
#define XASH_MOBILE_PLATFORM 1
#endif

//================================================================
//
//           ENDIANNESS DEFINES
//
//================================================================

#if defined(XASH_LITTLE_ENDIAN) && defined(XASH_BIG_ENDIAN)
#error "Both XASH_LITTLE_ENDIAN and XASH_BIG_ENDIAN are defined"
#endif

#if !defined(XASH_LITTLE_ENDIAN) || !defined(XASH_BIG_ENDIAN)
#if defined XASH_MSVC || __LITTLE_ENDIAN__
//!!! Probably all WinNT installations runs in little endian
#define XASH_LITTLE_ENDIAN 1
#elif __BIG_ENDIAN__
#define XASH_BIG_ENDIAN 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && defined(__ORDER_LITTLE_ENDIAN__) // some compilers define this
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define XASH_BIG_ENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define XASH_LITTLE_ENDIAN 1
#else
#error "Unknown endianness!"
#endif
#else
#include <sys/param.h>
#if __BYTE_ORDER == __BIG_ENDIAN
#define XASH_BIG_ENDIAN 1
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define XASH_LITTLE_ENDIAN 1
#else
#error "Unknown endianness!"
#endif
#endif // !XASH_WIN32
#endif

//================================================================
//
//           CPU ARCHITECTURE DEFINES
//
//================================================================
#if defined(__x86_64__) || defined(_M_X64)
#define XASH_64BIT 1
#define XASH_AMD64 1
#elif defined(__i386__) || defined(_X86_) || defined(_M_IX86)
#define XASH_X86 1
#elif defined __aarch64__
#define XASH_64BIT 1
#define XASH_ARM64 1
#elif defined __arm__ || defined _M_ARM
#if defined			 _M_ARM
#define XASH_ARM 7 // MSVC can only ARMv7
#elif __ARM_ARCH == 7
#define XASH_ARM 7
#elif __ARM_ARCH == 6
#define XASH_ARM 6
#elif __ARM_ARCH == 5
#define XASH_ARM 5
#elif __ARM_ARCH == 4
#define XASH_ARM 4
#else
#error "Unknown ARM"
#endif

#if defined _M_ARM
#error "No WinMobile port yet! Need to determine which ARM float ABI msvc uses if applicable"
#endif

#if defined __SOFTFP__ || __ARM_PCS_VFP == 0
#define XASH_ARM_SOFTFP 1
#else // __SOFTFP__
#define XASH_ARM_HARDFP 1
#endif // __SOFTFP__
#elif defined __mips__
#define XASH_MIPS 1
#elif defined __EMSCRIPTEN__
#define XASH_JS 1
#elif defined __e2k__
#define XASH_64BIT 1
#define XASH_E2K   1
#else
#error "Place your architecture name here! If this is a mistake, try to fix conditions above and report a bug"
#endif

#if defined(XASH_WAF_DETECTED_64BIT) && !defined(XASH_64BIT)
#define XASH_64BIT 1
#endif

//==========================================================//
// Feature test macros
//==========================================================//
#if ( defined(__AVX__) ) && !( defined(FORBID_SIMD) || defined(FORBID_AVX) )
#	undef USE_AVX
#	define USE_AVX 1
#endif
#if ( defined(__AVX2__) ) && !( defined(FORBID_SIMD) || defined(FORBID_AVX2) )
#	undef USE_AVX2
#	define USE_AVX2 1
#endif
#if ( defined(__SSE__) || defined(_M_X64) || (_M_IX86_FP >= 1) ) &&  !( defined(FORBID_SIMD) || defined(FORBID_SSE) )
#	undef USE_SSE
#	define USE_SSE 1
#endif
#if ( defined(__SSE2__) || defined(_M_X64) || (_M_IX86_FP >= 2) ) &&  !( defined(FORBID_SIMD) || defined(FORBID_SSE2) )
#	undef USE_SSE2
#	define USE_SSE2 1
#endif
#if ( defined(__SSE3__) || defined(_M_X64) ) &&  !( defined(FORBID_SIMD) || defined(FORBID_SSE3) )
#	undef USE_SSE3
#	define USE_SSE3 1
#endif
#if ( defined(__SSE4_1__) ) &&  !( defined(FORBID_SIMD) || defined(FORBID_SSE41) )
#	undef USE_SSE41
#	define USE_SSE41 1
#endif
#if ( defined(__SSE4_2__) ) &&  !( defined(FORBID_SIMD) || defined(FORBID_SSE42) )
#	undef USE_SSE42
#	define USE_SSE42 1
#endif
#if ( defined(__SSSE3__) ) &&  !( defined(FORBID_SIMD) || defined(FORBID_SSSE3) )
#	undef USE_SSSE3
#	define USE_SSSE3 1
#endif
#if ( defined(__ARM_NEON) ) && !( defined(FORBID_SIMD) || defined(FORBID_NEON) )
#	undef USE_NEON
#	define USE_NEON 1
#endif
#if ( defined(__ARM_FEATURE_SVE) ) && !( defined(FORBID_SIMD) || defined(FORBID_SVE) )
#	undef USE_SVE
#	define USE_SVE 1
#endif
#if ( defined(__ARM_FEATURE_SVE2) ) && !( defined(FORBID_SIMD) || defined(FORBID_SVE2) )
#	undef USE_SVE2
#	define USE_SVE2 1
#endif
#if ( defined(__ARM_FEATURE_CRC32) )
#	undef HAS_ARM_CRC32
#	define HAS_ARM_CRC32 1
#endif
#if ( defined(__ARM_FEATURE_FMA) )
#	undef HAS_ARM_FMA
#	define HAS_ARM_FMA 1
#endif
#if ( defined(__ARM_FEATURE_COMPLEX) )
#	undef HAS_ARM_COMPLEX
#	define HAS_ARM_COMPLEX 1
#endif
#if ( defined(__ARM_FEATURE_SVE) )
#	undef HAS_ARM_SVE
#	define HAS_ARM_SVE 1
#endif
#if ( defined(__ARM_FEATURE_SVE2) )
#	undef HAS_ARM_SVE2
#	define HAS_ARM_SVE2 1
#endif
#if ( defined(__amd64__) || defined(_M_X64_) )
#	undef PLATFORM_64BITS
#	undef _X64_
#	undef PLATFORM_AMD64
#	define PLATFORM_64BITS 1
#	define _X64_
#	define PLATFORM_AMD64 1
#endif
#if ( defined(__i386__) || defined(_M_IX86_) )
#	undef PLATFORM_32BITS
#	undef PLATFORM_X86
#	undef _X86_
#	define PLATFORM_32BITS 1
#	define PLATFORM_X86 1
#	define _X86_ 1
#endif
#if ( defined(_M_ARM) || defined(__arm__) )
#	undef PLATFORM_32BITS
#	undef PLATFORM_ARM
#	undef PLATFORM_ARM32
#	define PLATFORM_32BITS 1
#	define PLATFORM_ARM 1
#	define PLATFORM_ARM32 1
#endif
#if ( defined(_M_ARM64) || defined(__aarch64__) )
#	undef PLATFORM_64BITS
#	undef PLATFORM_ARM
#	undef PLATFORM_ARM64
#	define PLATFORM_64BITS 1
#	define PLATFORM_ARM 1
#	define PLATFORM_ARM64 1
#endif
#if ( defined(__ppc64__) )
#	undef PLATFORM_64BITS
#	undef PLATFORM_PPC
#	undef PLATFORM_PPC64
#	define PLATFORM_64BITS 1
#	define PLATFORM_PPC 1
#	define PLATFORM_PPC64 1
#elif ( defined(__ppc__) )
#	undef PLATFORM_32BITS
#	undef PLATFORM_PPC
#	undef PLATFORM_PPC32
#	define PLATFORM_32BITS 1
#	define PLATFORM_PPC 1
#	define PLATFORM_PPC32 1
#endif
#if ( defined(__riscv) )
#	undef PLATFORM_64BITS
#	undef PLATFORM_RISCV
#	undef PLATFORM_RISCV64
#	define PLATFORM_64BITS 1
#	define PLATFORM_RISCV 1
#	define PLATFORM_RISCV64 1
#endif
#if ( defined(__GNUC__) )
#	undef COMPILER_GCC
#	define COMPILER_GCC 1
#endif
#if ( defined(_MSC_VER) )
#	undef COMPILER_MSVC
#	define COMPILER_MSVC 1
#endif
#if ( defined(__clang__) )
#	undef COMPILER_CLANG
#	define COMPILER_CLANG 1
#endif
#if ( defined(__linux__) )
#	undef OS_LINUX
#	undef OS_POSIX
#	define OS_POSIX 1
#	define OS_LINUX 1
#endif
#if ( defined(_WIN32) )
#	undef OS_WINDOWS
#	define OS_WINDOWS 1
#endif
#if ( defined(__APPLE__) )
#	undef OS_OSX
#	undef OS_POSIX
#	define OS_POSIX 1
#	define OS_OSX 1
#endif
#if ( defined(__ANDROID__) )
#	undef OS_ANDROID
#	undef OS_POSIX
#	define OS_POSIX 1
#	define OS_ANDROID
#endif
#if ( __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ )
#	undef PLATFORM_BIG_ENDIAN
#	define PLATFORM_BIG_ENDIAN 1
#else
#	undef PLATFORM_LITTLE_ENDIAN
#	define PLATFORM_LITTLE_ENDIAN 1
#endif

#ifdef OS_POSIX
#undef _POSIX
#define _POSIX 1
#endif

#ifdef OS_LINUX
#undef _LINUX
#define _LINUX 1
#endif

#endif // BUILD_H
