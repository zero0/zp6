//
// Created by phosg on 2/7/2022.
//

#ifndef ZP_RENDERPIPELINE_H
#define ZP_RENDERPIPELINE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"

#include "Engine/JobSystem.h"

namespace zp
{


    class RenderPipelineContext
    {
    public:
        void beginRenderPass() {}

        void endRenderPass() {}

    public:
        GraphicsDevice* m_graphicsDevice;
        JobSystem* m_jobSystem;
    };

    ZP_DECLSPEC_NOVTABLE class RenderPipeline
    {
    public:
        virtual void onActivate( GraphicsDevice* graphicsDevice ) = 0;

        virtual void onDeactivate( GraphicsDevice* graphicsDevice ) = 0;

        virtual PreparedJobHandle onProcessPipeline( RenderPipelineContext* renderPipelineContext, const PreparedJobHandle& inputHandle ) = 0;
    };
};

#endif //ZP_RENDERPIPELINE_H
