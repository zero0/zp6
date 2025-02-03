//
// Created by phosg on 1/31/2025.
//

#include "Rendering/GraphicsCommandBuffer.h"

namespace zp
{
    GraphicsCommandBuffer::GraphicsCommandBuffer( MemoryLabel memoryLabel, zp_size_t pageSize )
        : m_data( memoryLabel, pageSize )
        , memoryLabel( memoryLabel )
    {
    }

    Memory GraphicsCommandBuffer::Data() const
    {
        return m_data.memory();
    }

    void GraphicsCommandBuffer::UpdateBufferData( CommandQueueHandle cmdQueue, BufferHandle dstBuffer, zp_size_t dstOffset, Memory srcData )
    {
        if( srcData.size >= 1024 )
        {
            m_data.write( CommandHeader { .type = CommandType::UpdateBufferDataExternal } );
            m_data.write( CommandUpdateBufferDataExternal {
                .cmdQueue = cmdQueue,
                .dstBuffer = dstBuffer,
                .dstOffset = dstOffset,
                .srcData = srcData
            } );
        }
        else
        {
            m_data.write( CommandHeader { .type = CommandType::UpdateBufferData } );
            m_data.write( CommandUpdateBufferData {
                .cmdQueue = cmdQueue,
                .dstBuffer = dstBuffer,
                .dstOffset = dstOffset,
                .srcLength = srcData.size
            } );
            m_data.write( srcData.ptr, srcData.size );
            m_data.writeAlignment( 8 );
        }
    }
};
