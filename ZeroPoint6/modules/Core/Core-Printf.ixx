module;

#include <stdio.h>
#include <stdarg.h>

#include "Core/Defines.h"

#if ZP_DEBUG
#include <debugapi.h>
#endif

export module Core:Printf;

export
{
    void zp_printf( const char* text, ...)
    {
        char szBuff[1024];
        va_list arg;
        va_start(arg, text);
        _vsnprintf(szBuff, sizeof(szBuff), text, arg);
        va_end(arg);


        printf_s(szBuff);

#if ZP_DEBUG
        OutputDebugString(szBuff);
#endif
    }
};