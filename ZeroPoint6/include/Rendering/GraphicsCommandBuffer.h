//
// Created by phosg on 1/31/2022.
//

#ifndef ZP_GRAPHICSCOMMANDBUFFER_H
#define ZP_GRAPHICSCOMMANDBUFFER_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"

namespace zp
{
    class GraphicsCommandBuffer
    {
    ZP_NONCOPYABLE( GraphicsCommandBuffer );

    public:
        GraphicsCommandBuffer( void* memory, zp_size_t capacity );
        ~GraphicsCommandBuffer();

    private:
        zp_uint8_t* m_commandBuffer;
        zp_size_t m_commandBufferLength;
        zp_size_t m_commandBufferCapacity;

        zp_int32_t m_index;
    };
}

#endif //ZP_GRAPHICSCOMMANDBUFFER_H
