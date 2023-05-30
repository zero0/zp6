//
// Created by phosg on 5/29/2023.
//

#ifndef ZP_SAMPLER_H
#define ZP_SAMPLER_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Math.h"

#include "Rendering/GraphicsResource.h"
#include "Rendering/GraphicsDefines.h"

namespace zp
{
    struct SamplerCreateDesc
    {
        FilterMode magFilter;
        FilterMode minFilter;
        MipmapMode mipmapMode;
        SamplerAddressMode addressModeU;
        SamplerAddressMode addressModeV;
        SamplerAddressMode addressModeW;
        CompareOp compareOp;
        BorderColor borderColor;
        zp_float32_t mipLodBias;
        zp_float32_t maxAnisotropy;
        zp_float32_t minLod;
        zp_float32_t maxLod;
        ZP_BOOL32( anisotropyEnabled );
        ZP_BOOL32( compareEnabled );
        ZP_BOOL32( unnormalizedCoordinates );
    };

    struct Sampler
    {
        zp_handle_t samplerHandle;
    };

    typedef GraphicsResource<Sampler> SamplerResource;
    typedef GraphicsResourceHandle<Sampler> SamplerResourceHandle;
}

#endif //ZP_SAMPLER_H
