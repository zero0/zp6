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

    void GraphicsCommandBuffer::UpdateTexture( CommandBufferHandle commandBuffer, Memory srcMemory, TextureHandle dstTexture, zp_uint32_t dstMipLevel, zp_uint32_t dstArrayLayer )
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
