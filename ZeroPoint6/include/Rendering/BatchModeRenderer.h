//
// Created by phosg on 2/24/2022.
//

#ifndef ZP_BATCHMODERENDERER_H
#define ZP_BATCHMODERENDERER_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Allocator.h"
#include "Core/Vector.h"

#include "Rendering/GraphicsDevice.h"
#include "Rendering/Material.h"

namespace zp
{
    enum
    {
        kMaxCachedPerFrame = 2
    };

    class GraphicsDevice;

    struct BatchModeRenderCommandHandle
    {
    private:
        zp_uint32_t m_id;

        friend class BatchModeRenderer;
    };

    struct BatchModeRenderCommand
    {
        GraphicsBuffer vertexBuffer;
        GraphicsBuffer indexBuffer;
        MaterialResourceHandle material;
        zp_uint32_t id;
        zp_uint32_t vertexBufferOffset;
        zp_uint32_t vertexStride;
        zp_uint32_t firstVertex;
        zp_uint32_t vertexCount;
        zp_int32_t vertexOffset;
        zp_uint32_t indexBufferOffset;
        zp_uint32_t firstIndex;
        zp_uint32_t indexCount;
        zp_uint32_t instanceCount;
        zp_uint32_t firstInstance;
        Matrix4x4f localToWorld;
        Bounds3Df localBounds;
        Bounds3Df worldBounds;
    };

    struct BatchModeCullingResults;

    struct BatchModeRendererConfig
    {
    };

    class BatchModeRenderer
    {
    public:
        BatchModeRenderer( MemoryLabel memoryLabel, const BatchModeRendererConfig* batchModeRendererConfig );

        ~BatchModeRenderer();

        void beginFrame( zp_uint64_t frameIndex );

        void process( GraphicsDevice* graphicsDevice );

        BatchModeRenderCommandHandle acquirePersistentRenderCommand();

        void releasePersistentRenderCommand( BatchModeRenderCommandHandle& handle );

        BatchModeRenderCommand* getPersistentRenderCommand( const BatchModeRenderCommandHandle& handle );

        BatchModeRenderCommand* requestTempRenderCommand();

    private:
        struct PerFrameData
        {
            GraphicsBufferAllocator perDrawAllocator;
        };

        PerFrameData m_perFrameData[kMaxCachedPerFrame];
        zp_uint64_t m_currentFrame;

        Vector<BatchModeRenderCommand> m_tempBatchRenderCommands;
        Vector<BatchModeRenderCommand> m_persistentBatchRenderCommands;
        Vector<zp_size_t> m_persistentBatchRenderIDToIndex;


    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_BATCHMODERENDERER_H
