//
// Created by phosg on 1/31/2022.
//

#ifndef ZP_IMMEDIATEMODERENDERER_H
#define ZP_IMMEDIATEMODERENDERER_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Allocator.h"
#include "Core/Vector.h"

#include "Rendering/GraphicsDefines.h"
#include "Rendering/GraphicsDevice.h"
#include "Rendering/Material.h"

namespace zp
{
    class GraphicsDevice;

    class BatchModeRenderer;

    struct VertexVUC
    {
        Vector3f vertexOS;
        Vector2f uv0;
        Color color;
    };

    struct ImmediateModeRendererConfig
    {
        zp_size_t commandCount;
        zp_size_t vertexCount;
        zp_size_t indexCount;
        GraphicsDevice* graphicsDevice;
        BatchModeRenderer* batchModeRenderer;
    };

    class ImmediateModeRenderer
    {
    ZP_NONCOPYABLE( ImmediateModeRenderer );

    public:
        ImmediateModeRenderer( MemoryLabel memoryLabel, const ImmediateModeRendererConfig* immediateModeRendererConfig );

        ~ImmediateModeRenderer();

        zp_uint32_t registerMaterial( const MaterialResourceHandle& materialResourceHandle );

        void beginFrame( zp_uint64_t frameIndex );

        zp_handle_t begin( zp_uint32_t materialIndex, Topology topology, zp_size_t vertexCount, zp_size_t indexCount );

        void setLocalToWorld( zp_handle_t command, const Matrix4x4f& localToWorld );

        template<zp_size_t Size>
        void addTriangles( zp_handle_t command, const VertexVUC (& vertices)[Size] )
        {
            static_assert( Size > 0 && Size % 3 == 0 );
            addTriangles( command, vertices, Size );
        }

        void addTriangles( zp_handle_t command, const VertexVUC* vertices, zp_size_t count );

        template<zp_size_t Size>
        void addQuads( zp_handle_t command, const VertexVUC (& vertices)[Size] )
        {
            static_assert( Size > 0 && Size % 4 == 0 );
            addQuads( command, vertices, Size );
        }

        void addQuads( zp_handle_t command, const VertexVUC* vertices, zp_size_t count );

        void end( zp_handle_t command );

        void process( BatchModeRenderer* batchModeRenderer );

    private:
        struct PerFrameData
        {
            zp_uint8_t* commandBuffer;
            zp_uint8_t* scratchVertexBuffer;
            zp_uint16_t* scratchIndexBuffer;
            zp_size_t commandBufferLength;
            zp_size_t commandBufferCapacity;
            zp_size_t vertexBufferOffset;
            zp_size_t vertexBufferCapacity;
            zp_size_t indexBufferLength;
            zp_size_t indexBufferCapacity;
            GraphicsBuffer vertexBuffer;
            GraphicsBuffer indexBuffer;
        };

        enum
        {
            kMaxBufferedImmediateFrames = 2
        };

        PerFrameData m_perFrameData[kMaxBufferedImmediateFrames];
        GraphicsBuffer m_vertexGraphicsBuffer;
        GraphicsBuffer m_indexGraphicsBuffer;

        zp_uint64_t m_currentFrame;

        GraphicsDevice* m_graphicsDevice;

        Vector<MaterialResourceHandle> m_registeredMaterials;

    public:
        MemoryLabel memoryLabel;
    };
}

#endif //ZP_IMMEDIATEMODERENDERER_H
