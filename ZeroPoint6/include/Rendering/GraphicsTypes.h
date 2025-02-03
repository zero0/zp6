//
// Created by phosg on 1/31/2025.
//

#ifndef ZP_GRAPHICSTYPES_H
#define ZP_GRAPHICSTYPES_H

#include "Core/Types.h"
#include "Core/Memory.h"

namespace zp
{
    enum class GraphicsHandleType
    {
        CommandQueue,
        Buffer,
        Texture,
        Shader,
    };

    template<GraphicsHandleType Type>
    struct GraphicsHandle
    {
        zp_uint32_t index;
        zp_hash32_t hash;

        GraphicsHandleType type() const
        {
            return Type;
        }
    };

    using CommandQueueHandle = GraphicsHandle<GraphicsHandleType::CommandQueue>;
    using TextureHandle = GraphicsHandle<GraphicsHandleType::Texture>;
    using BufferHandle = GraphicsHandle<GraphicsHandleType::Buffer>;

    //
    // TODO: move to internal
    //

    enum class CommandType : zp_uint32_t
    {
        None,
        UpdateBufferData,
        UpdateBufferDataExternal,
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
};

#endif //ZP_GRAPHICSTYPES_H
