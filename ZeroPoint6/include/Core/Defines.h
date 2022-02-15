//
// Created by phosg on 11/5/2021.
//

#ifndef ZP_DEFINES_H
#define ZP_DEFINES_H

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
#define ZP_DEBUG                    1
#endif

#if defined(RELEASE_BUILD)
#define ZP_RELEASE_BUILD            1
#endif

#if defined(WIN64) || defined(_WIN64)
#define ZP_ARCH64                   1
#endif

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#define ZP_OS_WINDOWS                      1
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


#define ZP_USE_CONSOLE_COLORS   1


#define ZP_USE_ASSERTIONS       1
#define ZP_USE_PRINTF           1
#define ZP_USE_SAFE_DELETE      1

#define ZP_USE_PROFILER         1

#endif //ZP_DEFINES_H
