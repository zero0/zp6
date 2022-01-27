//
// Created by phosg on 11/10/2021.
//

#ifndef ZP_APPLICATIONENTRYPOINT_H
#define ZP_APPLICATIONENTRYPOINT_H

namespace zp
{
    typedef void (* OnApplicationFocused)();

    typedef void (* OnApplicationUnfocused)();

    struct ApplicationEntryPoint
    {
        OnApplicationFocused onApplicationFocused;
        OnApplicationUnfocused onApplicationUnfocused;
    };

    typedef const ApplicationEntryPoint* (__cdecl* GetApplicationEntryPoint)();
}

#endif //ZP_APPLICATIONENTRYPOINT_H
