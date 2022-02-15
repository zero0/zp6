//
// Created by phosg on 2/9/2022.
//

#ifndef ZP_RENDERPASS_H
#define ZP_RENDERPASS_H

#include "Core/Defines.h"
#include "Core/Types.h"

#include "Rendering/GraphicsResource.h"

namespace zp
{
    struct RenderPassDesc
    {
        const char* name;
    };

    struct RenderPass
    {
        zp_handle_t internalRenderPass;
    };

    typedef GraphicsResource<RenderPass> RenderPassResource;
    typedef GraphicsResourceHandle<RenderPass> RenderPassResourceHandle;
}

#endif //ZP_RENDERPASS_H
