//
// Created by phosg on 1/31/2025.
//

#ifndef ZP_GRAPHICSCOMMANDBUFFER_H
#define ZP_GRAPHICSCOMMANDBUFFER_H

#include "Core/Types.h"
#include "Core/Memory.h"
#include "Core/Data.h"

#include "GraphicsTypes.h"

namespace zp
{
    class GraphicsCommandBuffer
    {
    public:
        GraphicsCommandBuffer( MemoryLabel memoryLabel, zp_size_t pageSize );

        Memory Data() const;

        zp_uint64_t frame;

        CommandQueueHandle BeginCommandQueue();

        void SubmitCommandQueue( CommandQueueHandle cmdQueue );

        void UpdateBufferData( CommandQueueHandle cmdQueue, BufferHandle dstBuffer, zp_size_t dstOffset, Memory srcData );

    private:
        DataStreamWriter m_data;

    public:
        const MemoryLabel memoryLabel;
    };
};

#endif //ZP_GRAPHICSCOMMANDBUFFER_H
