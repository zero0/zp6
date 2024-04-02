//
// Created by phosg on 7/4/2023.
//

#ifndef ZP_GRAPHICS_BUFFER_H
#define ZP_GRAPHICS_BUFFER_H

#include "Rendering/GraphicsResource.h"
#include "Rendering/GraphicsDefines.h"

namespace zp
{
    struct GraphicsBufferDesc
    {
        const char* name;
        zp_size_t size;
        GraphicsBufferUsageFlags usageFlags;
        MemoryPropertyFlags memoryPropertyFlags;
    };

    struct GraphicsBufferAllocation
    {
        zp_handle_t buffer;
        zp_handle_t deviceMemory;

        const zp_size_t offset;
        const zp_size_t size;
    };

    struct GraphicsBuffer
    {
        zp_handle_t buffer;
        zp_handle_t deviceMemory;

        zp_size_t offset;
        zp_size_t size;
        zp_size_t alignment;

        GraphicsBufferUsageFlags usageFlags;
        zp_bool_t isVirtualBuffer;

        [[nodiscard]] GraphicsBuffer splitBuffer( zp_size_t startOffset, zp_size_t splitSize ) const
        {
            GraphicsBuffer splitBuffer {
                .buffer = buffer,
                .deviceMemory = deviceMemory,
                .offset = offset + startOffset,
                .size = splitSize,
                .alignment = alignment,
                .usageFlags = usageFlags,
            };
            return splitBuffer;
        }
    };


    typedef GraphicsResource <GraphicsBuffer> GraphicsBufferResource;

    typedef GraphicsResourceHandle <GraphicsBuffer> GraphicsBufferResourceHandle;


    struct GraphicsBufferAllocator
    {
        GraphicsBuffer graphicsBuffer;

        zp_size_t allocated;

        void reset()
        {
            allocated = 0;
        }

        GraphicsBufferAllocation allocate( zp_size_t size )
        {
            const zp_size_t alignedSize = ZP_ALIGN_SIZE( size, graphicsBuffer.alignment );
            ZP_ASSERT( ( graphicsBuffer.offset + allocated + alignedSize ) < graphicsBuffer.size );

            GraphicsBufferAllocation allocation {
                .buffer = graphicsBuffer.buffer,
                .deviceMemory = graphicsBuffer.deviceMemory,
                .offset = graphicsBuffer.offset + allocated,
                .size = alignedSize
            };
            allocated += alignedSize;

            return allocation;
        }
    };

    struct GraphicsBufferUpdateDesc
    {
        const void* data;
        zp_size_t size;
    };
};

#endif //ZP_GRAPHICS_BUFFER_H
