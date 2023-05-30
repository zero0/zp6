//
// Created by phosg on 5/29/2023.
//

#ifndef ZP_RENDERTARGET_H
#define ZP_RENDERTARGET_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Math.h"

#include "Rendering/GraphicsResource.h"
#include "Rendering/GraphicsDefines.h"

namespace zp
{
    struct RenderTargetCreateDesc
    {
        TextureDimension textureDimension;
        TextureFormat textureFormat;
        SampleCount samples;
        TextureUsage usage;
        MemoryPropertyFlags memoryPropertyFlags;

        Size3Di size;
        zp_uint32_t arrayLayers;
    };

    struct RenderTarget
    {
        TextureDimension textureDimension;
        TextureFormat textureFormat;
        SampleCount samples;
        TextureUsage usage;

        Size3Di size;

        zp_handle_t m_renderTargetTextureHandle;
        zp_handle_t m_renderTargetImageViewHandle;
        zp_handle_t m_renderTargetFrameBufferHandle;
    };

    typedef GraphicsResource<RenderTarget> RenderTargetResource;
    typedef GraphicsResourceHandle<RenderTarget> RenderTargetResourceHandle;
}

#endif //ZP_RENDERTARGET_H
