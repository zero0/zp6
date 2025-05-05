//
// Created by phosg on 2/17/2022.
//

#ifndef ZP_TEXTURE_H
#define ZP_TEXTURE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Math.h"
#include "Core/Data.h"

#include "Rendering/GraphicsResource.h"
#include "Rendering/GraphicsDefines.h"

namespace zp
{
    struct TextureData
    {
        Size3Du size;
        TextureDimension dimension;
        GraphicsFormat format;
        zp_uint32_t mipCount;
        zp_uint32_t layerCount;

        Memory data;
        MemoryLabel memoryLabel;
    };

    void AllocateTextureData( MemoryLabel memoryLabel, Memory externalTextureData, TextureData& data );

    void FreeTextureData( TextureData& data );

    //
    //
    //

    enum MipDownSampleFunction : zp_uint32_t
    {
        Point,
        Bilinear,
    };

    enum ComponentSwizzle : zp_uint8_t
    {
        Identity,
        R,
        G,
        B,
        A,
        Zero,
        One,
    };

    struct ColorSwizzle
    {
        ComponentSwizzle r;
        ComponentSwizzle g;
        ComponentSwizzle b;
        ComponentSwizzle a;
    };

    struct TextureExportConfig
    {
        Size3Du maxSize;
        TextureDimension dimension;
        GraphicsFormat format;

        zp_uint32_t baseMipLevel;
        zp_uint32_t baseArrayLayer;

        MipDownSampleFunction mipDownSampleFunction;

        ColorSwizzle componentSwizzle;

        zp_uint32_t fadeMipLevelStart;
        zp_uint32_t fadeMipLevelEnd;
        Color32 fadeMipLevelDstColor;

        zp_bool_t sRGB;
        zp_bool_t generateMipMaps;
        zp_bool_t fadeMipMaps;
    };

    struct RawTextureData
    {
        Size3Du size;
        zp_uint32_t componentCount;
        zp_uint32_t componentSize;
        MemoryArray<zp_uint8_t> data;
    };

    void ExportRawTextureData( const TextureExportConfig& config, const RawTextureData& srcData, DataStreamWriter& dstDataStream );

}

#endif //ZP_TEXTURE_H
