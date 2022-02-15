#ifndef ZP_MODULEENTRYPOINTAPI_H
#define ZP_MODULEENTRYPOINTAPI_H

#include "Core/Defines.h"
#include "Core/Types.h"

namespace zp
{
    class Engine;

    typedef void (* OnEnginePreInitialize)( Engine* );

    typedef void (* OnEnginePostInitialize)( Engine* );

    typedef void (* OnEnginePreDestroy)( Engine* );

    typedef void (* OnEnginePostDestroy)( Engine* );

    typedef void (* OnEngineStarted)( Engine* );

    typedef void (* OnEngineStopped)( Engine* );

    struct ModuleEntryPointAPI
    {
        OnEnginePreInitialize onEnginePreInitialize;
        OnEnginePostInitialize onEnginePostInitialize;

        OnEnginePreDestroy onEnginePreDestroy;
        OnEnginePostDestroy onEnginePostDestroy;

        OnEngineStarted onEngineStarted;
        OnEngineStopped onEngineStopped;
    };

    typedef ModuleEntryPointAPI* (* GetModuleEntryPoint)();
}

#endif //ZP_MODULEENTRYPOINTAPI_H
