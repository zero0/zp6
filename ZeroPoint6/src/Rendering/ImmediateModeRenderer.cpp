//
// Created by phosg on 2/24/2022.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Atomic.h"
#include "Core/Map.h"
#include "Core/Profiler.h"

#include "Engine/MemoryLabels.h"

#include "Rendering/ImmediateModeRenderer.h"
#include "Rendering/BatchModeRenderer.h"

namespace zp
{
    namespace
    {
        struct ImmediateRenderCommand
        {
            zp_uint8_t* vertexBuffer;
            zp_uint16_t* indexBuffer;
            zp_uint32_t vertexCount;
            zp_uint32_t indexCount;
            zp_uint32_t materialIndex;
            Bounds3Df localBounds;
            Matrix4x4f localToWorld;
            zp_uint16_t vertexStride;
            zp_uint16_t topology;
        };

        constexpr zp_size_t kCommandStride = sizeof( ImmediateRenderCommand );
    }

    ImmediateModeRenderer::ImmediateModeRenderer( MemoryLabel memoryLabel, const ImmediateModeRendererConfig* immediateModeRendererConfig )
        : m_perFrameData {}
        , m_vertexGraphicsBuffer {}
        , m_indexGraphicsBuffer {}
        , m_currentFrame( 0 )
        , m_graphicsDevice( immediateModeRendererConfig->graphicsDevice )
        , m_registeredMaterials( 4, memoryLabel )
        , memoryLabel( memoryLabel )
    {
        const zp_size_t commandBufferStride = immediateModeRendererConfig->commandCount * kCommandStride;
        const zp_size_t vertexBufferStride = immediateModeRendererConfig->vertexCount * sizeof( VertexVUC );
        const zp_size_t indexBufferStride = immediateModeRendererConfig->indexCount * sizeof( zp_uint16_t );

        auto commandBuffer = ZP_MALLOC_T_ARRAY( memoryLabel, zp_uint8_t, commandBufferStride * kMaxBufferedImmediateFrames );
        auto vertexBuffer = ZP_MALLOC_T_ARRAY( memoryLabel, zp_uint8_t, vertexBufferStride * kMaxBufferedImmediateFrames );
        auto indexBuffer = ZP_MALLOC_T_ARRAY( memoryLabel, zp_uint16_t, immediateModeRendererConfig->indexCount * kMaxBufferedImmediateFrames );

        GraphicsBufferDesc vertexBufferDesc {
            .name = "Immediate Mode Vertex Buffer",
            .size = vertexBufferStride * kMaxBufferedImmediateFrames,
            .usageFlags = ZP_GRAPHICS_BUFFER_USAGE_VERTEX_BUFFER | ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_DEST,
            .memoryPropertyFlags = ZP_MEMORY_PROPERTY_HOST_VISIBLE,
        };
        m_graphicsDevice->createBuffer( &vertexBufferDesc, &m_vertexGraphicsBuffer );

        GraphicsBufferDesc indexBufferDesc {
            .name = "Immediate Mode Index Buffer",
            .size = indexBufferStride * kMaxBufferedImmediateFrames,
            .usageFlags = ZP_GRAPHICS_BUFFER_USAGE_INDEX_BUFFER | ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_DEST,
            .memoryPropertyFlags = ZP_MEMORY_PROPERTY_HOST_VISIBLE,
        };
        m_graphicsDevice->createBuffer( &indexBufferDesc, &m_indexGraphicsBuffer );

        for( zp_size_t i = 0; i < kMaxBufferedImmediateFrames; ++i )
        {
            PerFrameData& perFrameData = m_perFrameData[ i ];
            perFrameData.commandBuffer = commandBuffer + ( i * commandBufferStride );
            perFrameData.scratchVertexBuffer = vertexBuffer + ( i * vertexBufferStride );
            perFrameData.scratchIndexBuffer = indexBuffer + ( i * immediateModeRendererConfig->indexCount );
            perFrameData.commandBufferLength = 0;
            perFrameData.commandStride = kCommandStride;
            perFrameData.commandBufferCapacity = commandBufferStride;
            perFrameData.vertexBufferOffset = 0;
            perFrameData.vertexBufferCapacity = vertexBufferStride;
            perFrameData.indexBufferLength = 0;
            perFrameData.indexBufferCapacity = immediateModeRendererConfig->indexCount;
            perFrameData.vertexBuffer = m_vertexGraphicsBuffer.splitBuffer( i * vertexBufferStride, vertexBufferStride );
            perFrameData.indexBuffer = m_indexGraphicsBuffer.splitBuffer( i * indexBufferStride, indexBufferStride );
        }
    }

    ImmediateModeRenderer::~ImmediateModeRenderer()
    {
        ZP_FREE_( memoryLabel, m_perFrameData[ 0 ].commandBuffer );
        ZP_FREE_( memoryLabel, m_perFrameData[ 0 ].scratchVertexBuffer );
        ZP_FREE_( memoryLabel, m_perFrameData[ 0 ].scratchIndexBuffer );

        for( PerFrameData& perFrameData : m_perFrameData )
        {
            perFrameData = {};
        }

        m_graphicsDevice->destroyBuffer( &m_vertexGraphicsBuffer );
        m_graphicsDevice->destroyBuffer( &m_indexGraphicsBuffer );

        m_registeredMaterials.clear();

        m_graphicsDevice = nullptr;
    }

    zp_uint32_t ImmediateModeRenderer::registerMaterial( const MaterialResourceHandle& materialResourceHandle )
    {
        const zp_uint32_t materialIndex = m_registeredMaterials.size();
        m_registeredMaterials.pushBack( materialResourceHandle );

        return materialIndex;
    }

    void ImmediateModeRenderer::beginFrame( zp_uint64_t frameIndex )
    {
        ZP_PROFILE_CPU_BLOCK();

        m_currentFrame = frameIndex;

        const zp_size_t frame = m_currentFrame % kMaxBufferedImmediateFrames;
        PerFrameData& perFrameData = m_perFrameData[ frame ];
        perFrameData.commandBufferLength = 0;
        perFrameData.vertexBufferOffset = 0;
        perFrameData.indexBufferLength = 0;
    }

    zp_handle_t ImmediateModeRenderer::begin( zp_uint32_t materialIndex, Topology topology, zp_size_t vertexCount, zp_size_t indexCount )
    {
        ZP_PROFILE_CPU_BLOCK();

        const zp_size_t frame = m_currentFrame % kMaxBufferedImmediateFrames;
        PerFrameData& perFrameData = m_perFrameData[ frame ];

        const zp_size_t vertexSize = vertexCount * sizeof( VertexVUC );
        const zp_size_t commandStride = perFrameData.commandStride;

        const zp_size_t commandOffset = Atomic::AddSizeT( &perFrameData.commandBufferLength, commandStride ) - commandStride;
        const zp_size_t vertexOffset = Atomic::AddSizeT( &perFrameData.vertexBufferOffset, vertexSize ) - vertexSize;
        const zp_size_t indexOffset = Atomic::AddSizeT( &perFrameData.indexBufferLength, indexCount ) - indexCount;

        zp_handle_t cmd = perFrameData.commandBuffer + commandOffset;

        ZP_ASSERT( perFrameData.commandBufferLength < perFrameData.commandBufferCapacity );
        ZP_ASSERT( perFrameData.vertexBufferOffset < perFrameData.vertexBufferCapacity );
        ZP_ASSERT( perFrameData.indexBufferLength < perFrameData.indexBufferCapacity );

        ImmediateRenderCommand* command = static_cast<ImmediateRenderCommand*>( cmd );
        command->vertexBuffer = perFrameData.scratchVertexBuffer + vertexOffset;
        command->indexBuffer = perFrameData.scratchIndexBuffer + indexOffset;
        command->vertexCount = 0;
        command->indexCount = 0;
        command->materialIndex = materialIndex;
        command->localToWorld = Matrix4x4f::identity;
        command->localBounds = Bounds3Df::invalid;
        command->vertexStride = sizeof( VertexVUC );
        command->topology = topology;

        return cmd;
    }

    void ImmediateModeRenderer::setLocalToWorld( zp_handle_t command, const Matrix4x4f& localToWorld )
    {
        ZP_PROFILE_CPU_BLOCK();

        ImmediateRenderCommand* cmd = static_cast<ImmediateRenderCommand*>( command );
        cmd->localToWorld = localToWorld;
    }

    void ImmediateModeRenderer::addTriangles( zp_handle_t command, const VertexVUC* vertices, zp_size_t count )
    {
        ZP_PROFILE_CPU_BLOCK();

        ImmediateRenderCommand* cmd = static_cast<ImmediateRenderCommand*>( command );
        ZP_ASSERT( cmd->topology == ZP_TOPOLOGY_TRIANGLE_LIST );
        ZP_ASSERT( count > 0 );
        ZP_ASSERT( count % 3 == 0 );

        for( zp_size_t i = 0; i < count; ++i )
        {
            cmd->localBounds.encapsulate( vertices[ i ].vertexOS );
        }

        const zp_size_t vertexSize = sizeof( VertexVUC ) * count;
        zp_memcpy( cmd->vertexBuffer + cmd->vertexCount * cmd->vertexStride, vertexSize, vertices, vertexSize );

        zp_size_t indexCount = cmd->indexCount;
        zp_size_t vertexCount = cmd->vertexCount;

        const zp_size_t triCount = count / 3;
        for( zp_size_t i = 0; i < triCount; ++i )
        {
            cmd->indexBuffer[ indexCount + 0 ] = vertexCount + 0;
            cmd->indexBuffer[ indexCount + 1 ] = vertexCount + 2;
            cmd->indexBuffer[ indexCount + 2 ] = vertexCount + 1;

            indexCount += 3;
            vertexCount += 3;
        }

        cmd->indexCount = indexCount;
        cmd->vertexCount = vertexCount;
    }

    void ImmediateModeRenderer::addQuads( zp_handle_t command, const VertexVUC* vertices, zp_size_t count )
    {
        ZP_PROFILE_CPU_BLOCK();

        ImmediateRenderCommand* cmd = static_cast<ImmediateRenderCommand*>( command );
        ZP_ASSERT( cmd->topology == ZP_TOPOLOGY_TRIANGLE_LIST );
        ZP_ASSERT( count > 0 );
        ZP_ASSERT( count % 4 == 0 );

        for( zp_size_t i = 0; i < count; ++i )
        {
            cmd->localBounds.encapsulate( vertices[ i ].vertexOS );
        }

        const zp_size_t vertexSize = sizeof( VertexVUC ) * count;
        zp_memcpy( cmd->vertexBuffer + cmd->vertexCount * cmd->vertexStride, vertexSize, vertices, vertexSize );

        zp_size_t indexCount = cmd->indexCount;
        zp_size_t vertexCount = cmd->vertexCount;

        const zp_size_t quadCount = count / 4;
        for( zp_size_t i = 0; i < quadCount; ++i )
        {
            cmd->indexBuffer[ indexCount + 0 ] = vertexCount + 0;
            cmd->indexBuffer[ indexCount + 1 ] = vertexCount + 2;
            cmd->indexBuffer[ indexCount + 2 ] = vertexCount + 1;

            cmd->indexBuffer[ indexCount + 3 ] = vertexCount + 0;
            cmd->indexBuffer[ indexCount + 4 ] = vertexCount + 3;
            cmd->indexBuffer[ indexCount + 5 ] = vertexCount + 2;

            indexCount += 6;
            vertexCount += 4;
        }

        cmd->indexCount = indexCount;
        cmd->vertexCount = vertexCount;
    }

    void ImmediateModeRenderer::end( zp_handle_t command )
    {
    }

    void ImmediateModeRenderer::process( BatchModeRenderer* batchModeRenderer )
    {
        ZP_PROFILE_CPU_BLOCK();

        const zp_size_t frame = m_currentFrame % kMaxBufferedImmediateFrames;
        PerFrameData& perFrameData = m_perFrameData[ frame ];

        if( perFrameData.commandBufferLength > 0 && perFrameData.vertexBufferOffset > 0 && perFrameData.indexBufferLength > 0 )
        {
            GraphicsBufferUpdateDesc updateVertexBufferDesc {
                .data = perFrameData.scratchVertexBuffer,
                .dataSize = perFrameData.vertexBufferOffset,
            };

            GraphicsBufferUpdateDesc updateIndexBufferDesc {
                .data = perFrameData.scratchIndexBuffer,
                .dataSize = perFrameData.indexBufferLength * sizeof( zp_uint16_t ),
            };

            ImmediateRenderCommand* beginCommand = static_cast<ImmediateRenderCommand*>(static_cast<void*>(perFrameData.commandBuffer));
            ImmediateRenderCommand* endCommand = static_cast<ImmediateRenderCommand*>(static_cast<void*>(perFrameData.commandBuffer + perFrameData.commandBufferLength));

            void* vertexMemory = nullptr;
            void* indexMemory = nullptr;
            m_graphicsDevice->mapBuffer( 0, perFrameData.vertexBufferOffset, perFrameData.vertexBuffer, &vertexMemory );
            m_graphicsDevice->mapBuffer( 0, perFrameData.indexBufferLength * sizeof( zp_uint16_t ), perFrameData.indexBuffer, &indexMemory );

            zp_uint8_t* vertexBuffer = static_cast<zp_uint8_t*>( vertexMemory );
            zp_uint8_t* indexBuffer = static_cast<zp_uint8_t*>( indexMemory );

            zp_size_t vertexWriteOffset = 0;
            zp_size_t indexWriteOffset = 0;

            for( ImmediateRenderCommand* command = beginCommand; command != endCommand; ++command )
            {
                BatchModeRenderCommand* batchModeRenderCommand = batchModeRenderer->requestTempRenderCommand();
                batchModeRenderCommand->vertexBuffer = perFrameData.vertexBuffer;
                batchModeRenderCommand->indexBuffer = perFrameData.indexBuffer;
                batchModeRenderCommand->material = m_registeredMaterials[ command->materialIndex ];
                batchModeRenderCommand->vertexBufferOffset = vertexWriteOffset;
                batchModeRenderCommand->vertexStride = command->vertexStride;
                batchModeRenderCommand->firstVertex = 0;
                batchModeRenderCommand->vertexCount = command->vertexCount;
                batchModeRenderCommand->vertexOffset = 0;
                batchModeRenderCommand->indexBufferOffset = indexWriteOffset;
                batchModeRenderCommand->firstIndex = 0;
                batchModeRenderCommand->indexCount = command->indexCount;
                batchModeRenderCommand->instanceCount = 1;
                batchModeRenderCommand->firstInstance = 0;
                batchModeRenderCommand->localToWorld = command->localToWorld;
                batchModeRenderCommand->localBounds = command->localBounds;
                batchModeRenderCommand->worldBounds = Math::Mul( command->localToWorld, command->localBounds );

                zp_size_t cmdVertexSize = command->vertexCount * command->vertexStride;
                zp_size_t cmdIndexSize = command->indexCount * sizeof( zp_uint16_t );

                zp_memcpy( vertexBuffer, cmdVertexSize, command->vertexBuffer, cmdVertexSize );
                zp_memcpy( indexBuffer, cmdIndexSize, command->indexBuffer, cmdIndexSize );

                vertexBuffer += cmdVertexSize;
                indexBuffer += cmdIndexSize;

                vertexWriteOffset += cmdVertexSize;
                indexWriteOffset += cmdIndexSize;
            }

            m_graphicsDevice->unmapBuffer( perFrameData.vertexBuffer );
            m_graphicsDevice->unmapBuffer( perFrameData.indexBuffer );
        }
    }
}