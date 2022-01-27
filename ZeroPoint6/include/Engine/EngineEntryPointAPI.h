#ifndef ZP_ENGINEENTRYPOINTAPI_H
#define ZP_ENGINEENTRYPOINTAPI_H

namespace zp
{
    struct EngineEntryPointAPI
    {
        int value;
    };

    typedef const EngineEntryPointAPI* (__cdecl *GetEngineEntryPoint)();
}

#endif //ZP_ENGINEENTRYPOINTAPI_H
