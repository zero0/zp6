//
// Created by phosg on 1/31/2025.
//

#include "Core/Types.h"
#include "Core/Memory.h"

#include "Rendering/GraphicsTypes.h"
#include "Rendering/GraphicsCommandBuffer.h"

namespace zp
{
    namespace
    {
        enum
        {
            kUpdateBufferInlineAlignment = 8,
            kUpdateBufferInlineMaxSize = 1024,
            kPushConstantInlineAlignment = 8,
            kPushConstantInlineMaxSize = 256,
        };
    }

    GraphicsCommandBuffer::GraphicsCommandBuffer( MemoryLabel memoryLabel, zp_size_t pageSize )
        : m_data( memoryLabel, pageSize )
        , memoryLabel( memoryLabel )
    {
    }

    Memory GraphicsCommandBuffer::Data() const
    {
        return m_data.memory();
    }

    void GraphicsCommandBuffer::Reset()
    {
        m_data.reset();
    }


    BufferHandle GraphicsCommandBuffer::RequestBuffer( BufferHandle bufferHandle, const RequestBufferInfo& info )
    {
        if( !bufferHandle.valid() )
        {

        }
        return bufferHandle;
    }

    void GraphicsCommandBuffer::UpdateBuffer( CommandBufferHandle commandBuffer, BufferHandle dstBuffer, zp_size_t dstOffset, const Memory& srcData )
    {
        if( srcData.size < kUpdateBufferInlineMaxSize )
        {
            m_data.write( CommandHeader { .type = CommandType::UpdateBufferData } );
            m_data.write( CommandUpdateBufferData {
                .commandBuffer = commandBuffer,
                .dstBuffer = dstBuffer,
                .dstOffset = dstOffset,
                .srcLength = srcData.size
            } );
            m_data.write( srcData.ptr, srcData.size );
            m_data.writeAlignment( kUpdateBufferInlineAlignment );
        }
        else
        {
            m_data.write( CommandHeader { .type = CommandType::UpdateBufferDataExternal } );
            m_data.write( CommandUpdateBufferDataExternal {
                .commandBuffer = commandBuffer,
                .dstBuffer = dstBuffer,
                .dstOffset = dstOffset,
                .srcData = srcData
            } );
        }
    }

    void GraphicsCommandBuffer::UpdateTexture( CommandBufferHandle commandBuffer, const Memory srcMemory, TextureHandle dstTexture, zp_uint32_t dstMipLevel, zp_uint32_t dstArrayLayer )
    {
        m_data.write( CommandHeader { .type = CommandType::UpdateTextureData } );
        m_data.write( CommandUpdateTextureData {
            .commandBuffer = commandBuffer,
            .srcData = srcMemory,
            .dstTexture = dstTexture,
            .dstMipLevel = dstMipLevel,
            .dstArrayLayer = dstArrayLayer,
        } );
    }

    RenderPassHandle GraphicsCommandBuffer::BeginRenderPass( CommandBufferHandle commandBuffer, RenderPassHandle renderPassHandle )
    {
        return {};
    }

    void GraphicsCommandBuffer::EndRenderPass( CommandBufferHandle commandBuffer, RenderPassHandle renderPassHandle )
    {

    }

    PipelineHandle GraphicsCommandBuffer::RequestPipeline( const RequestPipelineInfo& info )
    {
        return {};
    }


    void GraphicsCommandBuffer::BindPipeline( CommandBufferHandle commandBuffer, PipelineHandle pipelineHandle )
    {
        m_data.write( CommandHeader { .type = CommandType::BindPipeline } );
        m_data.write( CommandBindPipeline {
            .commandBuffer = commandBuffer,
            .pipeline = pipelineHandle,
        } );
    }

    void GraphicsCommandBuffer::CopyBufferToBuffer( CommandBufferHandle commandBuffer, BufferHandle srcBuffer, BufferHandle dstBuffer, zp_size_t srcOffset, zp_size_t dstOffset, zp_size_t size )
    {
        m_data.write( CommandHeader { .type = CommandType::CopyBuffer } );
        m_data.write( CommandCopyBuffer {
            .commandBuffer = commandBuffer,
            .srcBuffer = srcBuffer,
            .dstBuffer = dstBuffer,
            .srcOffset = srcOffset,
            .dstOffset = dstOffset,
            .size = size,
        } );
    }

    void GraphicsCommandBuffer::PushConstant( CommandBufferHandle commandBuffer, PipelineHandle pipline, int shaderStage, zp_uint32_t offset, const Memory& srcMemory )
    {
        if( srcMemory.size < kPushConstantInlineMaxSize )
        {
            m_data.write( CommandHeader { .type = CommandType::PushConstant } );
            m_data.write( CommandPushConstant {
                .commandBuffer = commandBuffer,
                .pipeline = pipline,
                .offset = offset,
                .size = srcMemory.size,
                .shaderStage = shaderStage,
            } );
            m_data.write( srcMemory.ptr, srcMemory.size );
            m_data.writeAlignment( kPushConstantInlineAlignment );
        }
        else
        {
            m_data.write( CommandHeader { .type = CommandType::PushConstantExternal } );
            m_data.write( CommandPushConstantExternal {
                .commandBuffer = commandBuffer,
                .pipeline = pipline,
                .offset = offset,
                .srcData = srcMemory,
                .shaderStage = shaderStage,
            } );
        }
    }

    void GraphicsCommandBuffer::Dispatch( CommandBufferHandle commandBuffer, zp_uint32_t groupCountX, zp_uint32_t groupCountY, zp_uint32_t groupCountZ )
    {
        m_data.write( CommandHeader { .type = CommandType::Dispatch } );
        m_data.write( CommandDispatch {
            .commandBuffer = commandBuffer,
            .groupCountX = groupCountX,
            .groupCountY = groupCountY,
            .groupCountZ = groupCountZ,
        } );
    }

    void GraphicsCommandBuffer::DispatchIndirect( CommandBufferHandle commandBuffer, BufferHandle indirectBuffer, zp_size_t offset )
    {
        m_data.write( CommandHeader { .type = CommandType::DispatchIndirect } );
        m_data.write( CommandDispatchIndirect {
            .commandBuffer = commandBuffer,
            .buffer = indirectBuffer,
            .offset = offset,
        } );
    }

    void GraphicsCommandBuffer::Draw( CommandBufferHandle commandBuffer, zp_uint32_t vertexCount, zp_uint32_t instanceCount, zp_uint32_t firstVertex, zp_uint32_t firstInstance )
    {
        m_data.write( CommandHeader { .type = CommandType::Draw } );
        m_data.write( CommandDraw {
            .commandBuffer = commandBuffer,
            .vertexCount = vertexCount,
            .instanceCount = instanceCount,
            .firstVertex = firstVertex,
            .firstInstance = firstInstance,
        } );
    }

    CommandBufferHandle GraphicsCommandBuffer::BeginCommandBuffer( RenderQueue queue )
    {
        m_data.write( CommandHeader { .type = CommandType::BeginCommandBuffer } );
        m_data.write( CommandBeginCommandBuffer {
            .queue = queue,
        } );

        return {};
    }

    void GraphicsCommandBuffer::SubmitCommandBuffer( CommandBufferHandle commandBuffer )
    {
        m_data.write( CommandHeader { .type = CommandType::SubmitCommandBuffer } );
        m_data.write( CommandSubmitCommandBuffer {
            .commandBuffer = commandBuffer,
        } );
    }
};
