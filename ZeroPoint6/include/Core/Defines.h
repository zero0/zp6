//
// Created by phosg on 11/5/2021.
//

#ifndef ZP_DEFINES_H
#define ZP_DEFINES_H

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
#define ZP_DEBUG                    1
#else
#define ZP_DEBUG                    0
#endif

#if defined(DEBUG_BUILD)
#define ZP_DEBUG_BUILD              1
#define ZP_RELEASE_BUILD            0
#define ZP_DISTRIBUTION_BUILD       0
#elif defined(RELEASE_BUILD)
#define ZP_DEBUG_BUILD              0
#define ZP_RELEASE_BUILD            1
#define ZP_DISTRIBUTION_BUILD       0
#elif defined(DISTRIBUTION_BUILD)
#define ZP_DEBUG_BUILD              0
#define ZP_RELEASE_BUILD            0
#define ZP_DISTRIBUTION_BUILD       1
#else
#error "Unknown build type"
#endif

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#define ZP_PLATFORM_WINDOWS         1

#if defined(WIN64) || defined(_WIN64)
#define ZP_PLATFORM_ARCH64          1
#endif
#endif

#if defined(__cplusplus)
#if defined(_MSC_VER)
#define ZP_MSC                      1
#define ZP_DECLSPEC_ALIGN(x)        __declspec(align(x))
#define ZP_DECLSPEC_NOVTABLE        __declspec(novtable)
#define ZP_FORCEINLINE              __forceinline
#define ZP_DEPRECATED               __declspec(deprecated)
#elif defined(__GNUC__)
#define ZP_GNUC                     1
#define ZP_DECLSPEC_ALIGN(x)        __attribute__((aligned(x)))
#define ZP_DECLSPEC_NOVTABLE
#define ZP_FORCEINLINE              __attribute__((always_inline)) inline
#define ZP_DEPRECATED               __attribute__((deprecated))
#else
#define ZP_DECLSPEC_ALIGN(x)
#define ZP_DECLSPEC_NOVTABLE
#define ZP_FORCEINLINE              inline
#define ZP_DEPRECATED
#endif
#endif

#if __cpp_char8_t
#define ZP_USE_UTF8_LITERALS    1
#else
#define ZP_USE_UTF8_LITERALS    0
#endif

#define ZP_ALIGN16              ZP_DECLSPEC_ALIGN(16)

#include <Core/Config.h>

#define ZP_USE_CONSOLE_COLORS   1

#define ZP_USE_ASSERTIONS       1
#define ZP_USE_PRINTF           1
#define ZP_USE_SAFE_DELETE      1
#define ZP_USE_SAFE_FREE        1

#define ZP_USE_PROFILER         1
#define ZP_USE_MEMORY_PROFILER  1

#define ZP_USE_TESTS            1

#endif //ZP_DEFINES_H
