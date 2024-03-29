//
// Created by phosg on 2/7/2022.
//

#ifndef ZP_RENDERPIPELINE_H
#define ZP_RENDERPIPELINE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Job.h"

namespace zp
{
    class RenderSystem;

    class RenderPipelineContext
    {
    public:
        void beginRenderPass()
        {
        }

        void endRenderPass()
        {
        }

        void cull();

        void drawRenderers();

    public:
        zp_uint64_t m_frameIndex;
        GraphicsDevice* m_graphicsDevice;
        RenderSystem* m_renderSystem;
    };

    ZP_DECLSPEC_NOVTABLE class RenderPipeline
    {
    public:
        virtual void onActivate( RenderSystem* renderSystem ) = 0;

        virtual void onDeactivate( RenderSystem* renderSystem ) = 0;

        virtual JobHandle onProcessPipeline( RenderPipelineContext* renderPipelineContext, const JobHandle& parentJobHandle ) = 0;
    };
};

#endif //ZP_RENDERPIPELINE_H
