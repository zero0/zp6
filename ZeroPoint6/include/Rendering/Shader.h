//
// Created by phosg on 11/7/2021.
//

#ifndef ZP_SHADER_H
#define ZP_SHADER_H

#include "Core/Defines.h"
#include "Core/Types.h"

#include "Rendering/GraphicsResource.h"

namespace zp
{
    enum
    {
        ShaderStage_Count = 6,
        kMaxShaderEntryPointNameSize = 32,
    };

    enum ShaderStage
    {
        ZP_SHADER_STAGE_VERTEX,
        ZP_SHADER_STAGE_TESSELLATION_CONTROL,
        ZP_SHADER_STAGE_TESSELLATION_EVALUATION,
        ZP_SHADER_STAGE_GEOMETRY,
        ZP_SHADER_STAGE_FRAGMENT,
        ZP_SHADER_STAGE_COMPUTE,
    };

    struct ShaderDesc
    {
        const char* name;

        const char* entryPointName;
        zp_size_t codeSize;
        const void* codeData;

        ShaderStage shaderStage;
    };

    struct Shader
    {
        zp_hash128_t shaderHash;

        zp_handle_t shader;

        ShaderStage shaderStage;
        char entryPointName[kMaxShaderEntryPointNameSize];
    };

    typedef GraphicsResource <Shader> ShaderResource;
    typedef GraphicsResourceHandle <Shader> ShaderResourceHandle;
}

#endif //ZP_SHADER_H
