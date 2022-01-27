//
// Created by phosg on 1/11/2022.
//

#include <cstdio>
#include <cstdarg>
#include <cstring>

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"

#if ZP_DEBUG
#include <debugapi.h>
#endif

enum
{
    kPrintBufferSize = 1024
};

void zp_printf( const char* text, ... )
{
    char szBuff[kPrintBufferSize];
    va_list arg;
    va_start( arg, text );
    _vsnprintf( szBuff, sizeof( szBuff ), text, arg );
    va_end( arg );

    printf_s( szBuff );

#if ZP_DEBUG
    OutputDebugString(szBuff);
#endif
}

void zp_printfln( const char* text, ... )
{
    char szBuff[kPrintBufferSize];
    va_list arg;
    va_start( arg, text );
    _vsnprintf( szBuff, sizeof( szBuff ), text, arg );
    va_end( arg );

    printf_s( "%s\n", szBuff );

#if ZP_DEBUG
    OutputDebugString(szBuff);
#endif
}

void zp_memcpy( void* dst, zp_size_t dstLength, const void* src, zp_size_t srcLength )
{
    memcpy_s( dst, dstLength, src, srcLength );
}

void zp_memset( void* dst, zp_size_t dstLength, zp_int32_t value )
{
    memset( dst, value, dstLength );
}