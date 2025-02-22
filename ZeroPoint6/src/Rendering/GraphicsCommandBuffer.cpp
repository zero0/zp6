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
};
