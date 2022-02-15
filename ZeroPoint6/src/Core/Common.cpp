//
// Created by phosg on 1/11/2022.
//

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"

#ifdef ZP_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#endif // ZP_OS_WINDOWS

#if ZP_DEBUG

#include <debugapi.h>

#endif

enum
{
    kPrintBufferSize = 1024 * 2
};

zp_int32_t zp_printf( const char* text, ... )
{
    char szBuff[kPrintBufferSize];
    va_list arg;
    va_start( arg, text );
    const zp_int32_t write = _vsnprintf_s( szBuff, kPrintBufferSize, text, arg );
    va_end( arg );


#if !ZP_DEBUG
    OutputDebugString( szBuff );
#else
    printf_s( szBuff );
#endif

    return write;
}

zp_int32_t zp_printfln( const char* text, ... )
{
    char szBuff[kPrintBufferSize];

    va_list arg;
    va_start( arg, text );
    const zp_int32_t write = _vsnprintf_s( szBuff, kPrintBufferSize, text, arg );
    va_end( arg );

    szBuff[ write ] = '\n';
    szBuff[ write + 1 ] = '\0';


#if !ZP_DEBUG
    OutputDebugString( szBuff );
#else
    printf_s( szBuff );
#endif

    return write;
}

zp_int32_t zp_snprintf( char* dest, zp_size_t destSize, const char* format, ... )
{
    va_list args;
    va_start( args, format );
    const zp_int32_t write = _vsnprintf_s( dest, destSize, destSize, format, args );
    va_end( args );

    return write;
}

void zp_memcpy( void* dst, zp_size_t dstLength, const void* src, zp_size_t srcLength )
{
    memcpy_s( dst, dstLength, src, srcLength );
}

void zp_memset( void* dst, zp_size_t dstLength, zp_int32_t value )
{
    memset( dst, value, dstLength );
}

zp_time_t zp_time_now()
{
    zp_time_t time;

#if ZP_OS_WINDOWS
    LARGE_INTEGER val;
    QueryPerformanceCounter( &val );
    time = val.QuadPart;
#endif

    return time;
}

zp_uint64_t zp_time_cycle()
{
    zp_uint64_t cycles;

#if ZP_OS_WINDOWS
    cycles = __rdtsc();
#endif

    return cycles;
}

zp_uint32_t zp_current_thread_id()
{
    zp_uint32_t id;

#if ZP_OS_WINDOWS
    id = GetCurrentThreadId();
#endif

    return id;
}

void zp_yield_current_thread()
{
#if ZP_OS_WINDOWS
    YieldProcessor();
#endif
}

void zp_sleep_current_thread( zp_uint64_t timeMilliseconds )
{
#if ZP_OS_WINDOWS
    ::Sleep( timeMilliseconds );
#endif
}

void zp_debug_break()
{
#if ZP_DEBUG
#if ZP_OS_WINDOWS
    DebugBreak();
#endif
#endif
}

zp_int32_t zp_strcmp( const char* lh, const char* rh )
{
    return strcmp( lh, rh );
}
