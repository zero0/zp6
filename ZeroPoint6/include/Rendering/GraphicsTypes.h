//
// Created by phosg on 1/31/2025.
//

#ifndef ZP_GRAPHICSTYPES_H
#define ZP_GRAPHICSTYPES_H

#include "Core/Types.h"
#include "Core/Math.h"
#include "Core/Memory.h"

#include "Rendering/GraphicsDefines.h"

namespace zp
{
    enum class GraphicsHandleType
    {
        CommandQueue,
        Buffer,
        Texture,
        RenderTarget,
        Shader,
        BindSet,
        RenderPass,

        _Count,
    };

    enum class RenderResourceType : zp_uint32_t
    {
        Buffer,
        Texture
    };

    class RenderResourceHandle
    {
    public:
        RenderResourceHandle( zp_size_t index, zp_uint32_t version, RenderResourceType type, zp_bool_t readOnly )
            : m_handle( 0
                        | ( index & kIndexWriteMask ) << kIndexOffset
                        | ( ( version % ( kVersionWriteMask - 1 ) ) + 1 ) << kVersionOffset
                        | ( static_cast<zp_uint32_t>( type ) & kTypeWriteMask ) << kTypeOffset
                        | ( readOnly ? kReadOnlyWriteMask : 0 ) << kReadOnlyOffset
        )
        {
        }

        [[nodiscard]] auto valid() const -> zp_bool_t
        {
            return m_handle != 0;
        }

        [[nodiscard]] auto index() const -> zp_size_t
        {
            return static_cast<zp_size_t>( ( m_handle & kIndexReadMask ) >> kIndexOffset );
        }

        [[nodiscard]] auto version() const -> zp_uint32_t
        {
            return ( m_handle & kVersionReadMask ) >> kVersionOffset;
        }

        [[nodiscard]] auto type() const -> RenderResourceType
        {
            return static_cast<RenderResourceType>( ( m_handle & kTypeReadMask ) >> kTypeOffset );
        }

        [[nodiscard]] auto readonly() const -> zp_bool_t
        {
            return ( m_handle & kReadOnlyReadMask ) == kReadOnlyReadMask;
        }

        [[nodiscard]] operator zp_uint32_t() const
        {
            return m_handle;
        }

    private:
        enum : zp_uint32_t
        {
            kIndexOffset = 0,
            kIndexWidth = 16,
            kIndexWriteMask = ( ( 1U << kIndexWidth ) - 1 ),
            kIndexReadMask = kIndexWriteMask << kIndexOffset,

            kVersionOffset = kIndexWidth,
            kVersionWidth = 12,
            kVersionWriteMask = ( ( 1U << kVersionWidth ) - 1 ),
            kVersionReadMask = kVersionWriteMask << kVersionOffset,

            kTypeOffset = kIndexWidth + kVersionWidth,
            kTypeWidth = 3,
            kTypeWriteMask = ( ( 1U << kTypeWidth ) - 1 ),
            kTypeReadMask = kTypeWriteMask << kTypeOffset,

            kReadOnlyOffset = kIndexWidth + kVersionWidth + kTypeWidth,
            kReadOnlyWidth = 1,
            kReadOnlyWriteMask = ( ( 1U << kReadOnlyWidth ) - 1 ),
            kReadOnlyReadMask = kReadOnlyWriteMask << kReadOnlyOffset,
        };

        zp_uint32_t m_handle;
    };

    template<GraphicsHandleType Type>
    struct GraphicsHandle
    {
        using value_type = GraphicsHandle<Type>;
        using const_reference_type = const value_type&;

        static value_type Null;

        zp_uint32_t index;
        zp_hash32_t hash;

        [[nodiscard]] GraphicsHandleType type() const
        {
            return Type;
        }

        [[nodiscard]] zp_bool_t valid() const
        {
            return index != 0 && hash != 0;
        }

        zp_bool_t operator==( const_reference_type other ) const
        {
            return index == other.index && hash == other.hash;
        }
    };

    using CommandQueueHandle = GraphicsHandle<GraphicsHandleType::CommandQueue>;
    using TextureHandle = GraphicsHandle<GraphicsHandleType::Texture>;
    using RenderTargetHandle = GraphicsHandle<GraphicsHandleType::RenderTarget>;
    using ShaderHandle = GraphicsHandle<GraphicsHandleType::Shader>;
    using BufferHandle = GraphicsHandle<GraphicsHandleType::Buffer>;
    using BindSetHandle = GraphicsHandle<GraphicsHandleType::BindSet>;
    using RenderPassHandle = GraphicsHandle<GraphicsHandleType::RenderPass>;

    //
    // TODO: move to internal
    //

    enum class CommandType : zp_uint32_t
    {
        None,
        UpdateBufferData,
        UpdateBufferDataExternal,

        CopyBuffer,

        BeginRenderPass,
        NextSubpass,
        EndRenderPass,

        Dispatch,
        DispatchIndirect,

        Draw,
        DrawIndexed,
        DrawIndirect,
        DrawIndexedIndirect,

        Blit,

        BeginCommandQueue,
        SubmitCommandQueue,
    };

    struct CommandHeader
    {
        CommandType type;
    };

    struct CommandUpdateBufferData
    {
        CommandQueueHandle cmdQueue;
        BufferHandle dstBuffer;
        zp_size_t dstOffset;
        zp_size_t srcLength;
    };

    struct CommandUpdateBufferDataExternal
    {
        CommandQueueHandle cmdQueue;
        BufferHandle dstBuffer;
        zp_size_t dstOffset;
        Memory srcData;
    };

    struct CommandCopyBuffer
    {
        CommandQueueHandle cmdQueue;
        BufferHandle srcBuffer;
        BufferHandle dstBuffer;
        zp_size_t srcOffset;
        zp_size_t dstOffset;
        zp_size_t size;
    };

    struct CommandBeginRenderPass
    {
        struct ColorAttachment
        {
            RenderTargetHandle attachment;
            AttachmentLoadOp loadOp;
            AttachmentStoreOp storeOp;
            Color clearColor;
        };

        struct DepthAttachment
        {
            RenderTargetHandle attachment;
            int loadOp;
            int storeOp;
            zp_float32_t clearDepth;
        };

        struct StencilAttachment
        {
            RenderTargetHandle attachment;
            int loadOp;
            int storeOp;
            zp_uint32_t clearStencil;
        };

        CommandQueueHandle cmdQueue;
        Rect2Du renderArea;

        DepthAttachment depthAttachment;
        StencilAttachment stencilAttachment;
        zp_uint32_t colorAttachmentCount;
    };

    struct CommandNextSubpass
    {
        CommandQueueHandle cmdQueue;
    };

    struct CommandEndRenderPass
    {
        CommandQueueHandle cmdQueue;
    };

    struct CommandDispatch
    {
        CommandQueueHandle cmdQueue;
        zp_uint32_t groupCountX;
        zp_uint32_t groupCountY;
        zp_uint32_t groupCountZ;
    };

    struct CommandDispatchIndirect
    {
        CommandQueueHandle cmdQueue;
        BufferHandle buffer;
        zp_size_t offset;
    };

    struct CommandDraw
    {
        CommandQueueHandle cmdQueue;
        zp_uint32_t vertexCount;
        zp_uint32_t instanceCount;
        zp_uint32_t firstVertex;
        zp_uint32_t firstInstance;
    };

    struct CommandDrawIndexed
    {
        CommandQueueHandle cmdQueue;
        zp_uint32_t indexCount;
        zp_uint32_t instanceCount;
        zp_uint32_t firstIndex;
        zp_int32_t vertexOffset;
        zp_uint32_t firstInstance;
    };

    struct CommandDrawIndirect
    {
        CommandQueueHandle cmdQueue;
        BufferHandle buffer;
        zp_size_t offset;
        zp_uint32_t drawCount;
        zp_uint32_t stride;
    };

    struct CommandDrawIndexedIndirect
    {
        CommandQueueHandle cmdQueue;
        BufferHandle buffer;
        zp_size_t offset;
        zp_uint32_t drawCount;
        zp_uint32_t stride;
    };

    struct CommandBlit
    {
        CommandQueueHandle cmdQueue;
        TextureHandle srcTexture;
        TextureHandle dstTexture;
        zp_uint32_t srcMip;
        zp_uint32_t dstMip;
        Rect2Di srcRegion;
        Rect2Di dstRegion;
    };

    struct CommandBeginCommandQueue
    {
        RenderQueue queue;
        CommandQueueHandle cmdQueue;
    };

    struct CommandSubmitCommandQueue
    {
        CommandQueueHandle cmdQueue;
    };
};

#endif //ZP_GRAPHICSTYPES_H
