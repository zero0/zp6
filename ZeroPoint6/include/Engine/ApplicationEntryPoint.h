//
// Created by phosg on 11/10/2021.
//

#ifndef ZP_APPLICATIONENTRYPOINT_H
#define ZP_APPLICATIONENTRYPOINT_H

namespace zp
{
    using OnApplicationFocused = void (* )();

    using OnApplicationUnfocused = void (* )();

    struct ApplicationEntryPoint
    {
        OnApplicationFocused onApplicationFocused;
        OnApplicationUnfocused onApplicationUnfocused;
    };

    using GetApplicationEntryPoint = ApplicationEntryPoint* (* )();
}

#endif //ZP_APPLICATIONENTRYPOINT_H
