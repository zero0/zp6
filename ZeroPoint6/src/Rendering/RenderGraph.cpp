//
// Created by phosg on 8/1/2024.
//

#include "Rendering/RenderGraph.h"

namespace zp
{
    RenderGraph::RenderGraph( Memory scratchMemory, MemoryLabel memoryLabel )
        : m_frameAllocator( { scratchMemory.ptr, scratchMemory.size }, {}, {} )
        , m_allPassData()
        , m_executions()
        , m_conditionals()
        , m_resourceCounts()
        , m_executionPassCount( 4 )
        , m_rasterPassCount( 16 )
        , m_computePassCount( 16 )
        , memoryLabel( memoryLabel )
    {
    }

    void RenderGraph::BeginRecording()
    {
        m_frameAllocator.policy().reset();

        m_allPassData = MemoryList<PassData>( AllocateFrameMemoryArray<PassData>( m_executionPassCount ) );

        m_resourceTexture = MemoryList<int>( AllocateFrameMemoryArray<int>( m_resourceCounts[ ( zp_size_t)RenderGraphResourceType::Texture ] ) );
        m_resourceRenderTexture = MemoryList<int>( AllocateFrameMemoryArray<int>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ] ) );
        m_resourceHistoryRenderTexture = MemoryList<int>( AllocateFrameMemoryArray<int>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::HistoryRenderTexture ] ) );
        m_resourceRenderList = MemoryList<RenderList>( AllocateFrameMemoryArray<RenderList>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderList ] ) );
        m_resourceBuffer = MemoryList<int>( AllocateFrameMemoryArray<int>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Buffer ] ) );
        m_resourceShader = MemoryList<int>( AllocateFrameMemoryArray<int>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Shader ] ) );
        m_resourceComputeShader = MemoryList<int>( AllocateFrameMemoryArray<int>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::ComputeShader ] ) );
        m_resourceFence = MemoryList<int>( AllocateFrameMemoryArray<int>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Fence ] ) );
        m_resourceColor = MemoryList<Color>( AllocateFrameMemoryArray<Color>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Color ] ) );
        m_resourceVector4 = MemoryList<Vector4f>( AllocateFrameMemoryArray<Vector4f>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Vector4 ] ) );
        m_resourceMatrix4x4 = MemoryList<Matrix4x4f>( AllocateFrameMemoryArray<Matrix4x4f>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Matrix4x4 ] ) );
        m_resourceInteger = MemoryList<zp_int32_t>( AllocateFrameMemoryArray<zp_int32_t>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Integer ] ) );
        m_resourceFloat = MemoryList<zp_float32_t>( AllocateFrameMemoryArray<zp_float32_t>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Float ] ) );
        m_resourceMemory = MemoryList<Memory>( AllocateFrameMemoryArray<Memory>( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Memory ] ) );

    }

    RenderTextureHandle RenderGraph::ImportBackBufferColor()
    {
        const zp_size_t index = m_resourceRenderTexture.length();
        m_resourceRenderTexture.push_back_uninitialized() = {};

        m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ] = zp_max( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ], m_resourceRenderTexture.length() );

        return { index, 0};
    }

    RenderTextureHandle RenderGraph::ImportBackBufferDepth()
    {
        const zp_size_t index = m_resourceRenderTexture.length();
        m_resourceRenderTexture.push_back_uninitialized() = {};

        m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ] = zp_max( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ], m_resourceRenderTexture.length() );

        return { index, 0 };
    }

    RenderTextureHandle RenderGraph::CreateRenderTexture( int renderTextureDesc )
    {
        const zp_size_t index = m_resourceRenderTexture.length();
        m_resourceRenderTexture.push_back_uninitialized() = {};

        m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ] = zp_max( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ], m_resourceRenderTexture.length() );

        return { index, 0 };
    }

    FloatHandle RenderGraph::CreateGlobalFloat( zp_float32_t value )
    {
        const zp_size_t index = m_resourceFloat.length();
        m_resourceFloat.push_back_uninitialized() = value;

        m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Float ] = zp_max( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Float ], m_resourceFloat.length() );

        return { index, 0 };
    }

    void RenderGraph::Present( const RenderTextureHandle& present )
    {
    }

    void RenderGraph::EndRecording()
    {
    }

    void RenderGraph::Compile( CompiledRenderGraph& compiledRenderGraph ) const
    {
    }

    Memory RenderGraph::AllocateFrameMemory( zp_size_t size )
    {
        return { .ptr = m_frameAllocator.allocate( size, kDefaultMemoryAlignment ), .size = size };
    }
}
