//
// Created by phosg on 1/31/2025.
//

#ifndef ZP_GRAPHICSCOMMANDBUFFER_H
#define ZP_GRAPHICSCOMMANDBUFFER_H

#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Memory.h"
#include "Core/Data.h"

#include "GraphicsTypes.h"

namespace zp
{
#if 0
    enum class ShaderStage : zp_uint32_t;

    constexpr ShaderStage operator|( ShaderStage lh, ShaderStage rh )
    {
        return (ShaderStage)( (zp_uint32_t)lh | (zp_uint32_t)rh );
    }

    constexpr ShaderStage operator&( ShaderStage lh, ShaderStage rh )
    {
        return (ShaderStage)( (zp_uint32_t)lh & (zp_uint32_t)rh );
    }

    constexpr ShaderStage operator^( ShaderStage lh, ShaderStage rh )
    {
        return (ShaderStage)( (zp_uint32_t)lh ^ (zp_uint32_t)rh );
    }

    ShaderStage& operator|=( ShaderStage& rh, ShaderStage lh )
    {
        rh = rh | lh;
        return rh;
    }

    ShaderStage& operator&=( ShaderStage& rh, ShaderStage lh )
    {
        rh = rh & lh;
        return rh;
    }

    ShaderStage& operator^=( ShaderStage& rh, ShaderStage lh )
    {
        rh = rh ^ lh;
        return rh;
    }

    enum class ShaderStage : zp_uint32_t
    {
        None = 0U,

        Vertex = 1 << 0,
        Geometry = 1 << 1,
        Fragment = 1 << 2,
        Compute = 1 << 3,
        Mesh = 1 << 4,
        Task = 1 << 5,

        AllGraphics = Vertex | Geometry | Fragment,

        All = ~0U,
    };
#endif
    struct RequestBufferInfo
    {
        String name;
        zp_size_t size;
        zp_size_t alignment;
        zp_uint32_t usage;
    };

    struct RequestBindSetInfo
    {

    };

    class GraphicsCommandBuffer
    {
    public:
        GraphicsCommandBuffer( MemoryLabel memoryLabel, zp_size_t pageSize );

        Memory Data() const;

        zp_uint64_t frame;

        BufferHandle RequestBuffer( BufferHandle bufferHandle, const RequestBufferInfo& info );

        void UpdateBuffer( CommandQueueHandle cmdQueue, BufferHandle dstBuffer, zp_size_t dstOffset, const Memory& srcData );

        template<typename T>
        void UpdateBuffer( CommandQueueHandle cmdQueue, BufferHandle dstBuffer, zp_size_t dstOffset, T&& srcData )
        {
            ZP_STATIC_ASSERT( sizeof( T ) % 16 == 0 );
            const Memory srcMem { .ptr = &srcData, .size = sizeof( T ) };
            UpdateBuffer( cmdQueue, dstBuffer, dstOffset, srcMem );
        }

        void CopyBufferToBuffer( CommandQueueHandle cmdQueue, BufferHandle srcBuffer, BufferHandle dstBuffer, zp_size_t srcOffset, zp_size_t dstOffset, zp_size_t size );

        void UpdateTexture( CommandQueueHandle cmdQueue, Memory srcMemory, TextureHandle dstTexture, zp_uint32_t dstMipLevel, zp_uint32_t dstArrayLayer );


        BindSetHandle RequestBindSet( BindSetHandle& bindSetHandle, const RequestBindSetInfo& info );

        void Bind( CommandQueueHandle cmdQueue, BindSetHandle bindSetHandle );


        RenderPassHandle BeginRenderPass( CommandQueueHandle cmdQueue, RenderPassHandle& renderPassHandle );

        void NextSubPass( CommandQueueHandle cmdQueue, RenderPassHandle renderPassHandle );

        void EndRenderPass( CommandQueueHandle cmdQueue, RenderPassHandle renderPassHandle );


        void PushConstant( CommandQueueHandle cmdQueue, BindSetHandle bindSetHandle, int shaderStage, zp_uint32_t offset, Memory memory );

        void Dispatch( CommandQueueHandle cmdQueue, zp_uint32_t groupCountX, zp_uint32_t groupCountY, zp_uint32_t groupCountZ );

        void DispatchIndirect( CommandQueueHandle cmdQueue, BufferHandle indirectBuffer, zp_size_t offset = 0 );

        void Draw( CommandQueueHandle cmdQueue, zp_uint32_t vertexCount, zp_uint32_t instanceCount, zp_uint32_t firstVertex, zp_uint32_t firstInstance );


        CommandQueueHandle BeginCommandQueue( RenderQueue queue );

        void SubmitCommandQueue( CommandQueueHandle cmdQueue );

    private:
        DataStreamWriter m_data;

    public:
        const MemoryLabel memoryLabel;
    };
};

#endif //ZP_GRAPHICSCOMMANDBUFFER_H
