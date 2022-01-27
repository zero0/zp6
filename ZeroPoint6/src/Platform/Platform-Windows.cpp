//
// Created by phosg on 12/11/2021.
//
module;
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <stdio.h>
#include <stdarg.h>

module Platform;

import Core;

namespace zp::Platform
{
    void Printf(const char* szFormat, ...)
    {

    }


    zp_int32_t ShowPopupModal(const char* title)
    {
        return 0;
    }

    zp_handle_t CreatePlatformWindow()
    {
        return ZP_NULL_HANDLE;
    }
}
