//
// Created by phosg on 1/31/2025.
//

#ifndef ZP_GRAPHICSCOMMANDBUFFER_H
#define ZP_GRAPHICSCOMMANDBUFFER_H

#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Memory.h"
#include "Core/Data.h"
#include "Core/Math.h"

#include "Rendering/GraphicsDefines.h"
#include "Rendering/GraphicsTypes.h"

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

    struct RequestPipelineInfo
    {
        String name;
        PipelineBindPoint pipelineBindPoint;
    };

    class GraphicsCommandBuffer
    {
    public:
        GraphicsCommandBuffer( MemoryLabel memoryLabel, zp_size_t pageSize );

        Memory Data() const;

        void Reset();

        zp_uint64_t frame;

        BufferHandle RequestBuffer( BufferHandle bufferHandle, const RequestBufferInfo& info );

        void UpdateBuffer( CommandBufferHandle commandBuffer, BufferHandle dstBuffer, zp_size_t dstOffset, const Memory& srcData );

        template<typename T>
        void UpdateBuffer( CommandBufferHandle commandBuffer, BufferHandle dstBuffer, zp_size_t dstOffset, T&& srcData )
        {
            ZP_STATIC_ASSERT( sizeof( T ) % 16 == 0 );
            const Memory srcMem { .ptr = &srcData, .size = sizeof( T ) };
            UpdateBuffer( commandBuffer, dstBuffer, dstOffset, srcMem );
        }

        void CopyBufferToBuffer( CommandBufferHandle commandBuffer, BufferHandle srcBuffer, BufferHandle dstBuffer, zp_size_t srcOffset, zp_size_t dstOffset, zp_size_t size );

        void UpdateTexture( CommandBufferHandle commandBuffer, Memory srcMemory, TextureHandle dstTexture, zp_uint32_t dstMipLevel, zp_uint32_t dstArrayLayer );


        PipelineHandle RequestPipeline( const RequestPipelineInfo& info );

        void BindPipeline( CommandBufferHandle commandBuffer, PipelineHandle pipelineHandle );


        RenderPassHandle BeginRenderPass( CommandBufferHandle commandBuffer, RenderPassHandle renderPassHandle );

        void NextSubPass( CommandBufferHandle commandBuffer, RenderPassHandle renderPassHandle );

        void EndRenderPass( CommandBufferHandle commandBuffer, RenderPassHandle renderPassHandle );

        template<typename T>
        void PushConstant( CommandBufferHandle commandBuffer, PipelineHandle pipline, int shaderStage, zp_uint32_t offset, T&& srcData )
        {
            ZP_STATIC_ASSERT( sizeof( T ) % 16 == 0 );
            const Memory srcMem { .ptr = &srcData, .size = sizeof( T ) };
            PushConstant( commandBuffer, pipline, shaderStage, offset, srcMem );
        }

        void PushConstant( CommandBufferHandle commandBuffer, PipelineHandle pipline, int shaderStage, zp_uint32_t offset, const Memory& memory );

        void Dispatch( CommandBufferHandle commandBuffer, const Size3Du& groupCount )
        {
            Dispatch( commandBuffer, groupCount.width, groupCount.height, groupCount.depth );
        }

        void Dispatch( CommandBufferHandle commandBuffer, zp_uint32_t groupCountX, zp_uint32_t groupCountY, zp_uint32_t groupCountZ );

        void DispatchIndirect( CommandBufferHandle commandBuffer, BufferHandle indirectBuffer, zp_size_t offset = 0 );


        void Draw( CommandBufferHandle commandBuffer, zp_uint32_t vertexCount, zp_uint32_t instanceCount = 1, zp_uint32_t firstVertex = 0, zp_uint32_t firstInstance = 0 );


        CommandBufferHandle BeginCommandBuffer( RenderQueue queue );

        void SubmitCommandBuffer( CommandBufferHandle commandBuffer );

    private:
        DataStreamWriter m_data;

    public:
        const MemoryLabel memoryLabel;
    };
};

#endif //ZP_GRAPHICSCOMMANDBUFFER_H
