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

        void UpdateBuffer( CommandBufferHandle cmdQueue, BufferHandle dstBuffer, zp_size_t dstOffset, const Memory& srcData );

        template<typename T>
        void UpdateBuffer( CommandBufferHandle cmdQueue, BufferHandle dstBuffer, zp_size_t dstOffset, T&& srcData )
        {
            ZP_STATIC_ASSERT( sizeof( T ) % 16 == 0 );
            const Memory srcMem { .ptr = &srcData, .size = sizeof( T ) };
            UpdateBuffer( cmdQueue, dstBuffer, dstOffset, srcMem );
        }

        void CopyBufferToBuffer( CommandBufferHandle cmdQueue, BufferHandle srcBuffer, BufferHandle dstBuffer, zp_size_t srcOffset, zp_size_t dstOffset, zp_size_t size );

        void UpdateTexture( CommandBufferHandle cmdQueue, Memory srcMemory, TextureHandle dstTexture, zp_uint32_t dstMipLevel, zp_uint32_t dstArrayLayer );


        BindSetHandle RequestBindSet( BindSetHandle& bindSetHandle, const RequestBindSetInfo& info );

        void Bind( CommandBufferHandle cmdQueue, BindSetHandle bindSetHandle );


        RenderPassHandle BeginRenderPass( CommandBufferHandle cmdQueue, RenderPassHandle& renderPassHandle );

        void NextSubPass( CommandBufferHandle cmdQueue, RenderPassHandle renderPassHandle );

        void EndRenderPass( CommandBufferHandle cmdQueue, RenderPassHandle renderPassHandle );


        void PushConstant( CommandBufferHandle cmdQueue, BindSetHandle bindSetHandle, int shaderStage, zp_uint32_t offset, Memory memory );

        void Dispatch( CommandBufferHandle cmdQueue, zp_uint32_t groupCountX, zp_uint32_t groupCountY, zp_uint32_t groupCountZ );

        void DispatchIndirect( CommandBufferHandle cmdQueue, BufferHandle indirectBuffer, zp_size_t offset = 0 );

        void Draw( CommandBufferHandle cmdQueue, zp_uint32_t vertexCount, zp_uint32_t instanceCount, zp_uint32_t firstVertex, zp_uint32_t firstInstance );


        CommandBufferHandle BeginCommandBuffer( RenderQueue queue );

        void SubmitCommandBuffer( CommandBufferHandle cmdQueue );

    private:
        DataStreamWriter m_data;

    public:
        const MemoryLabel memoryLabel;
    };
};

#endif //ZP_GRAPHICSCOMMANDBUFFER_H
