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

    void GraphicsCommandBuffer::UpdateBuffer( CommandQueueHandle cmdQueue, BufferHandle dstBuffer, zp_size_t dstOffset, const Memory& srcData )
    {
        if( srcData.size < kUpdateBufferInlineMaxSize )
        {
            m_data.write( CommandHeader { .type = CommandType::UpdateBufferData } );
            m_data.write( CommandUpdateBufferData {
                .cmdQueue = cmdQueue,
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
                .cmdQueue = cmdQueue,
                .dstBuffer = dstBuffer,
                .dstOffset = dstOffset,
                .srcData = srcData
            } );
        }
    }

    void GraphicsCommandBuffer::UpdateTexture( CommandQueueHandle cmdQueue, Memory srcMemory, TextureHandle dstTexture, zp_uint32_t dstMipLevel, zp_uint32_t dstArrayLayer )
    {
        m_data.write( CommandHeader { .type = CommandType::UpdateTextureData } );
        m_data.write( CommandUpdateTextureData {
            .cmdQueue = cmdQueue,
            .srcData = srcMemory,
            .dstTexture = dstTexture,
            .dstMipLevel = dstMipLevel,
            .dstArrayLayer = dstArrayLayer,
        } );
    }

    void GraphicsCommandBuffer::CopyBufferToBuffer( CommandQueueHandle cmdQueue, BufferHandle srcBuffer, BufferHandle dstBuffer, zp_size_t srcOffset, zp_size_t dstOffset, zp_size_t size )
    {
        m_data.write( CommandHeader { .type = CommandType::CopyBuffer } );
        m_data.write( CommandCopyBuffer {
            .cmdQueue = cmdQueue,
            .srcBuffer = srcBuffer,
            .dstBuffer = dstBuffer,
            .srcOffset = srcOffset,
            .dstOffset = dstOffset,
            .size = size,
        } );
    }

    void GraphicsCommandBuffer::Dispatch( CommandQueueHandle cmdQueue, zp_uint32_t groupCountX, zp_uint32_t groupCountY, zp_uint32_t groupCountZ )
    {
        m_data.write( CommandHeader { .type = CommandType::Dispatch } );
        m_data.write( CommandDispatch {
            .cmdQueue = cmdQueue,
            .groupCountX = groupCountX,
            .groupCountY = groupCountY,
            .groupCountZ = groupCountZ,
        } );
    }

    void GraphicsCommandBuffer::DispatchIndirect( CommandQueueHandle cmdQueue, BufferHandle indirectBuffer, zp_size_t offset )
    {
        m_data.write( CommandHeader { .type = CommandType::DispatchIndirect } );
        m_data.write( CommandDispatchIndirect {
            .cmdQueue = cmdQueue,
            .buffer = indirectBuffer,
            .offset = offset,
        } );
    }

    CommandQueueHandle GraphicsCommandBuffer::BeginCommandQueue( RenderQueue queue )
    {
        m_data.write( CommandHeader { .type = CommandType::BeginCommandQueue } );
        m_data.write( CommandBeginCommandQueue {
            .queue = queue,
        } );

        return {};
    }

    void GraphicsCommandBuffer::SubmitCommandQueue( CommandQueueHandle cmdQueue )
    {
        m_data.write( CommandHeader { .type = CommandType::SubmitCommandQueue } );
        m_data.write( CommandSubmitCommandQueue {
            .cmdQueue = cmdQueue,
        } );
    }
};
