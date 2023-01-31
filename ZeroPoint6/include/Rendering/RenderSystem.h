//
// Created by phosg on 11/6/2021.
//

#ifndef ZP_RENDERSYSTEM_H
#define ZP_RENDERSYSTEM_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Vector.h"
#include "Core/Allocator.h"
#include "Core/Math.h"

#include "Engine/JobSystem.h"
#include "Engine/EntityComponentManager.h"

#include "Rendering/GraphicsDevice.h"
#include "Rendering/RenderPipeline.h"

namespace zp
{
    class ImmediateModeRenderer;

    class BatchModeRenderer;

    class RenderSystem
    {
    ZP_NONCOPYABLE( RenderSystem );

    public:
        explicit RenderSystem( MemoryLabel memoryLabel );

        ~RenderSystem();

        void initialize( zp_handle_t windowHandle, GraphicsDeviceFeatures graphicsDeviceFeatures );

        void destroy();

        PreparedJobHandle startSystem( zp_uint64_t frameIndex, JobSystem* jobSystem, const PreparedJobHandle& inputHandle );

        PreparedJobHandle processSystem( zp_uint64_t frameIndex, JobSystem* jobSystem, EntityComponentManager* entityComponentManager, const PreparedJobHandle& inputHandle );

        GraphicsDevice* getGraphicsDevice()
        {
            return m_graphicsDevice;
        }

        ImmediateModeRenderer* getImmediateModeRenderer()
        {
            return m_immediateModeRenderer;
        }

        BatchModeRenderer* getBatchModeRenderer()
        {
            return m_batchModeRenderer;
        }

    private:
        GraphicsDevice* m_graphicsDevice;
        RenderPipeline* m_currentRenderPipeline;
        RenderPipeline* m_nextRenderPipeline;

        ImmediateModeRenderer* m_immediateModeRenderer;
        BatchModeRenderer* m_batchModeRenderer;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_RENDERSYSTEM_H
