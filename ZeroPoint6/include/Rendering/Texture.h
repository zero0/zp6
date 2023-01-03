//
// Created by phosg on 2/17/2022.
//

#ifndef ZP_TEXTURE_H
#define ZP_TEXTURE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Math.h"

#include "Rendering/GraphicsResource.h"
#include "Rendering/GraphicsDefines.h"

namespace zp
{
    struct TextureCreateDesc
    {
        TextureDimension textureDimension;
        TextureFormat textureFormat;
        SampleCount samples;
        TextureUsage usage;
        MemoryPropertyFlags memoryPropertyFlags;

        Size3Di size;
        zp_uint32_t mipCount;
        zp_uint32_t arrayLayers;
    };

    struct TextureMip
    {
        zp_size_t offset;
        zp_uint32_t width;
        zp_uint32_t height;
    };

    struct TextureUpdateDesc
    {
        const void* textureData;
        zp_size_t textureDataSize;

        TextureMip* mipLevels;

        Rect2Di extents;
        zp_uint32_t minMipLevel;
        zp_uint32_t maxMipLevel;
    };

    struct Texture
    {
        TextureDimension textureDimension;
        TextureFormat textureFormat;
        TextureUsage usage;

        Size3Di size;

        zp_handle_t textureHandle;
        zp_handle_t textureViewHandle;
        zp_handle_t textureMemoryHandle;
    };

    typedef GraphicsResource<Texture> TextureResource;
    typedef GraphicsResourceHandle<Texture> TextureResourceHandle;

    struct SamplerCreateDesc
    {
        FilterMode magFilter;
        FilterMode minFilter;
        MipmapMode mipmapMode;
        SamplerAddressMode addressModeU;
        SamplerAddressMode addressModeV;
        SamplerAddressMode addressModeW;
        CompareOp compareOp;
        zp_float32_t mipLodBias;
        zp_float32_t maxAnisotropy;
        zp_float32_t minLod;
        zp_float32_t maxLod;
        BorderColor borderColor;
        ZP_BOOL32( anisotropyEnabled );
        ZP_BOOL32( compareEnabled );
        ZP_BOOL32( unnormalizedCoordinates );
    };

    struct Sampler
    {
        zp_handle_t samplerHandle;
    };
}

#endif //ZP_TEXTURE_H
