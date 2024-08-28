//
// Created by phosg on 8/1/2024.
//

#include "Rendering/RenderGraph.h"

namespace zp
{
    RenderGraph::RenderGraph( Memory scratchMemory, MemoryLabel memoryLabel )
        : m_frameAllocator( { scratchMemory.ptr, scratchMemory.size }, {}, {} )
        , m_executionPasses()
        , m_rasterPasses()
        , m_computePasses()
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

        m_executionPasses = MemoryList<ExecutionPass>( AllocateFrameMemoryArray<ExecutionPass>( m_executionPassCount ) );
        m_rasterPasses = MemoryList<RasterPass>( AllocateFrameMemoryArray<RasterPass>( m_rasterPassCount ) );
        m_computePasses = MemoryList<ComputePass>( AllocateFrameMemoryArray<ComputePass>( m_computePassCount ) );


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

        return { .index = static_cast<zp_uint32_t>( index ) };
    }

    RenderTextureHandle RenderGraph::ImportBackBufferDepth()
    {
        const zp_size_t index = m_resourceRenderTexture.length();
        m_resourceRenderTexture.push_back_uninitialized() = {};

        m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ] = zp_max( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ], m_resourceRenderTexture.length() );

        return { .index = static_cast<zp_uint32_t>( index ) };
    }

    RenderTextureHandle RenderGraph::CreateRenderTexture( int renderTextureDesc )
    {
        const zp_size_t index = m_resourceRenderTexture.length();
        m_resourceRenderTexture.push_back_uninitialized() = {};

        m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ] = zp_max( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::RenderTexture ], m_resourceRenderTexture.length() );

        return { .index = static_cast<zp_uint32_t>( index ) };
    }

    FloatHandle RenderGraph::CreateGlobalFloat( zp_float32_t value )
    {
        const zp_size_t index = m_resourceFloat.length();
        m_resourceFloat.push_back_uninitialized() = value;

        m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Float ] = zp_max( m_resourceCounts[ (zp_size_t)RenderGraphResourceType::Float ], m_resourceFloat.length() );

        return { .index = static_cast<zp_uint32_t>( index ) };
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

    zp_size_t RenderGraph::RequestExecutionPass()
    {
        const zp_size_t index = m_executionPasses.length();
        m_executionPasses.push_back_empty();
        m_executionPassCount = zp_max( m_executionPasses.length(), m_executionPassCount );
        return index;
    }

    zp_size_t RenderGraph::RequestRasterPass()
    {
        const zp_size_t index = m_rasterPasses.length();
        m_rasterPasses.push_back_empty();
        m_rasterPassCount = zp_max( m_rasterPasses.length(), m_rasterPassCount );
        return index;
    }

    zp_size_t RenderGraph::RequestComputePass()
    {
        const zp_size_t index = m_computePasses.length();
        m_computePasses.push_back_empty();
        m_computePassCount = zp_max( m_computePasses.length(), m_computePassCount );
        return index;
    }

    void RenderGraph::RequestPassData( zp_size_t passDataSize, void** outPassData )
    {
        *outPassData = m_frameAllocator.allocate( passDataSize, kDefaultMemoryAlignment );
    }
}
