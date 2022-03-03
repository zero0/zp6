//
// Created by phosg on 3/1/2022.
//

#include "Core/Defines.h"
#include "Core/Common.h"
#include "Core/Allocator.h"
#include "Core/Profiler.h"

#include "Engine/MemoryLabels.h"

#include "Rendering/BatchModeRenderer.h"
#include "Rendering/GraphicsDevice.h"

namespace zp
{
    namespace
    {
        struct BatchRenderCommandGeneralSortKey
        {
            zp_uint64_t fullScreenLayer : 2;
            zp_uint64_t viewport : 3;
            zp_uint64_t viewportLayer : 3;
            zp_uint64_t translucencyType : 2;
            zp_uint64_t cmd : 1;

            zp_uint64_t padding : 53;
        };
        ZP_STATIC_ASSERT( sizeof( BatchRenderCommandGeneralSortKey ) == sizeof( zp_uint64_t ) );

        struct BatchRenderCommandDrawSortKey
        {
            zp_uint64_t fullScreenLayer : 2;
            zp_uint64_t viewport : 3;
            zp_uint64_t viewportLayer : 3;
            zp_uint64_t translucencyType : 2;
            zp_uint64_t cmd : 1;

            zp_uint64_t depth : 24;
            zp_uint64_t materialID : 20;
            zp_uint64_t pass : 9;
        };
        ZP_STATIC_ASSERT( sizeof( BatchRenderCommandDrawSortKey ) == sizeof( zp_uint64_t ) );

        struct BatchRenderCommandFuncSortKey
        {
            zp_uint64_t fullScreenLayer : 2;
            zp_uint64_t viewport : 3;
            zp_uint64_t viewportLayer : 3;
            zp_uint64_t translucencyType : 2;
            zp_uint64_t cmd : 1;

            zp_uint64_t sequence : 16;
            zp_uint64_t functionType : 5;
            zp_uint64_t functionPtr : 32;
        };
        ZP_STATIC_ASSERT( sizeof( BatchRenderCommandFuncSortKey ) == sizeof( zp_uint64_t ) );

        struct BatchRenderCommandSort
        {
            union
            {
                BatchRenderCommandGeneralSortKey generalSortKey;
                BatchRenderCommandDrawSortKey drawSortKey;
                BatchRenderCommandFuncSortKey funcSortKey;
            };

            const BatchModeRenderCommand* cmd;

            static zp_int32_t compare( const BatchRenderCommandSort& lh, const BatchRenderCommandSort& rh )
            {
                zp_int32_t cmp = 0;

                cmp = zp_cmp( lh.generalSortKey.fullScreenLayer, lh.generalSortKey.fullScreenLayer );
                cmp = cmp == 0 ? zp_cmp( lh.generalSortKey.viewport, rh.generalSortKey.viewport ) : cmp;
                cmp = cmp == 0 ? zp_cmp( lh.generalSortKey.viewportLayer, rh.generalSortKey.viewportLayer ) : cmp;
                cmp = cmp == 0 ? zp_cmp( lh.generalSortKey.translucencyType, rh.generalSortKey.translucencyType ) : cmp;

                if( zp_cmp( lh.generalSortKey.cmd, rh.generalSortKey.cmd ) == 0 )
                {
                    if( lh.generalSortKey.cmd )
                    {
                        cmp = cmp == 0 ? zp_cmp( lh.funcSortKey.sequence, rh.funcSortKey.sequence ) : cmp;
                    }
                    else
                    {
                        cmp = cmp == 0 ? zp_cmp( lh.drawSortKey.depth, rh.drawSortKey.depth ) : cmp;
                        cmp = cmp == 0 ? zp_cmp( lh.drawSortKey.materialID, rh.drawSortKey.materialID ) : cmp;
                        cmp = cmp == 0 ? zp_cmp( lh.drawSortKey.pass, rh.drawSortKey.pass ) : cmp;
                    }
                }

                return cmp;
            }
        };
    };

    BatchModeRenderer::BatchModeRenderer( MemoryLabel memoryLabel, const BatchModeRendererConfig* batchModeRendererConfig )
        : m_currentFrame( 0 )
        , m_tempBatchRenderCommands( 4, memoryLabel )
        , m_persistentBatchRenderCommands( 4, memoryLabel )
        , m_persistentBatchRenderIDToIndex( 4, memoryLabel )
        , memoryLabel( memoryLabel )
    {
    }

    BatchModeRenderer::~BatchModeRenderer()
    {
    }

    void BatchModeRenderer::beginFrame( zp_uint64_t frameIndex )
    {
        m_currentFrame = frameIndex;

        m_tempBatchRenderCommands.clear();
    }

    void BatchModeRenderer::process( GraphicsDevice* graphicsDevice )
    {
        ZP_PROFILE_CPU_BLOCK();

        // fill sort commands
        const zp_size_t sortCapacity = m_tempBatchRenderCommands.size() + m_persistentBatchRenderCommands.size();
        if( sortCapacity == 0 ) return;

        Vector<BatchRenderCommandSort> sortedBatchRenderCommands( sortCapacity, MemoryLabels::Temp );

        BatchRenderCommandSort sortCommand {};
        for( const BatchModeRenderCommand& cmd : m_tempBatchRenderCommands )
        {
            sortCommand.drawSortKey = {};
            sortCommand.cmd = &cmd;

            sortedBatchRenderCommands.pushBack( sortCommand );
        }

        for( const BatchModeRenderCommand& cmd : m_persistentBatchRenderCommands )
        {
            sortCommand.drawSortKey = {};
            sortCommand.cmd = &cmd;

            sortedBatchRenderCommands.pushBack( sortCommand );
        }

        // sort
        const StaticFunctionComparer<BatchRenderCommandSort> comparer {};
        zp_qsort3( sortedBatchRenderCommands.begin(), sortedBatchRenderCommands.end() - 1, comparer );

        zp_hash128_t prevMaterial {};

        // process commands
        CommandQueue* commandQueue = graphicsDevice->requestCommandQueue( ZP_RENDER_QUEUE_GRAPHICS, m_currentFrame );

        graphicsDevice->beginRenderPass( nullptr, commandQueue );

        for( const BatchRenderCommandSort& sort : sortedBatchRenderCommands )
        {
            if( sort.generalSortKey.cmd )
            {
                // process command

            }
            else
            {
                // process draw command
                const BatchModeRenderCommand* cmd = sort.cmd;
                if( cmd->material.isValid() )
                {
                    if( prevMaterial != cmd->material->materialRenderPipeline.pipelineHash )
                    {
                        graphicsDevice->bindPipeline( &cmd->material->materialRenderPipeline, ZP_PIPELINE_BIND_POINT_GRAPHICS, commandQueue );

                        prevMaterial = cmd->material->materialRenderPipeline.pipelineHash;
                    }

                    if( cmd->indexCount > 0 )
                    {
                        const GraphicsBuffer* vertexBuffers[] { &cmd->vertexBuffer };
                        graphicsDevice->bindVertexBuffers( 0, 1, vertexBuffers, nullptr, commandQueue );

                        graphicsDevice->bindIndexBuffer( &cmd->indexBuffer, ZP_INDEX_BUFFER_FORMAT_UINT16, cmd->indexBufferOffset, commandQueue );

                        graphicsDevice->drawIndexed( cmd->indexCount, cmd->instanceCount, cmd->firstIndex, cmd->vertexOffset, cmd->firstInstance, commandQueue );
                    }
                    else
                    {
                        const GraphicsBuffer* vertexBuffers[] { &cmd->vertexBuffer };
                        graphicsDevice->bindVertexBuffers( 0, 1, vertexBuffers, nullptr, commandQueue );

                        graphicsDevice->draw( cmd->vertexCount, cmd->instanceCount, cmd->firstVertex, cmd->firstInstance, commandQueue );
                    }
                }
            }
        }

        graphicsDevice->endRenderPass( commandQueue );
    }

    BatchModeRenderCommandHandle BatchModeRenderer::acquirePersistentRenderCommand()
    {
        ZP_PROFILE_CPU_BLOCK();

        const zp_uint32_t index = m_persistentBatchRenderCommands.size();

        BatchModeRenderCommand& command = m_persistentBatchRenderCommands.pushBackEmpty();
        command.id = index;

        while( m_persistentBatchRenderIDToIndex.size() <= index )
        {
            m_persistentBatchRenderIDToIndex.pushBack( -1 );
        }

        m_persistentBatchRenderIDToIndex[ index ] = index;

        BatchModeRenderCommandHandle handle {};
        handle.m_id = index;

        return handle;
    }

    void BatchModeRenderer::releasePersistentRenderCommand( BatchModeRenderCommandHandle& handle )
    {
        ZP_PROFILE_CPU_BLOCK();

        if( handle.m_id != -1 && handle.m_id < m_persistentBatchRenderIDToIndex.size() )
        {
            const zp_size_t index = m_persistentBatchRenderIDToIndex[ handle.m_id ];
            m_persistentBatchRenderIDToIndex[ handle.m_id ] = -1;

            m_persistentBatchRenderCommands.eraseAtSwapBack( index );

            m_persistentBatchRenderIDToIndex.back() = index;
        }

        handle.m_id = -1;
    }

    BatchModeRenderCommand* BatchModeRenderer::getPersistentRenderCommand( const BatchModeRenderCommandHandle& handle )
    {
        ZP_PROFILE_CPU_BLOCK();

        BatchModeRenderCommand* batchModeRenderCommand = nullptr;

        if( handle.m_id != -1 && handle.m_id < m_persistentBatchRenderIDToIndex.size() )
        {
            const zp_size_t index = m_persistentBatchRenderIDToIndex[ handle.m_id ];
            if( index != -1 )
            {
                batchModeRenderCommand = &m_persistentBatchRenderCommands[ index ];
            }
        }

        return batchModeRenderCommand;
    }

    BatchModeRenderCommand* BatchModeRenderer::requestTempRenderCommand()
    {
        ZP_PROFILE_CPU_BLOCK();

        const zp_uint32_t id = m_tempBatchRenderCommands.size();

        BatchModeRenderCommand& batchModeRenderCommand = m_tempBatchRenderCommands.pushBackEmpty();
        batchModeRenderCommand.id = 100000 + id;

        return &batchModeRenderCommand;
    }
}