//
// Created by phosg on 7/27/2024.
//

#ifndef ZP_RENDERGRAPH_H
#define ZP_RENDERGRAPH_H

#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Function.h"
#include "Core/Allocator.h"
#include "Core/Vector.h"

#include "Engine/MemoryLabels.h"

#include "Rendering/GraphicsDevice.h"

namespace zp
{
    class RenderList
    {
    };

    struct RenderTextureDesc
    {

    };

    class RasterCommandBuffer
    {
    public:
        void SetViewport( const Viewport& viewport );

        void SetScissorRect( const Rect2Di& scissorRect );

        void DisableScissorRect();

        void BindShader( int location, Shader* shader );

        void BindBuffer( int location, GraphicsBuffer* buffer );

        void DrawRenderList( RenderList* renderList );

        void DrawProcedural();

        void Execute();

        void Clear();
    };

    class ComputeCommandBuffer
    {
    public:
        template<typename T>
        void SetBufferData( GraphicsBuffer* buffer, const T& data, zp_size_t offset = 0 )
        {
            const void* ptr = &data;
            SetBufferData( buffer, ptr, sizeof( T ), offset );
        }

        void SetBufferData( GraphicsBuffer* buffer, const Memory& memory, zp_size_t offset = 0 )
        {
            SetBufferData( buffer, memory.ptr, memory.size, offset );
        }

        void SetBufferData( GraphicsBuffer* buffer, const void* data, zp_size_t size, zp_size_t offset = 0 );

        void BindBuffer( int location, GraphicsBuffer* buffer );

        void Dispatch( const int& computeShader, zp_uint32_t kernel, zp_uint32_t x, zp_uint32_t y, zp_uint32_t z );

        void Dispatch( const int& computeShader, zp_uint32_t kernel, const Size3Du& xyz );

        void Execute();

        void Clear();
    };

    enum class RenderTargetStoreAction
    {
        DontCare,
        Store,
        Resolve,
        StoreAndResolve,
    };

    enum class RenderTargetLoadAction
    {
        DontCare,
        Load,
        Clear,
    };

    enum class RenderGraphResourceType : zp_uint32_t
    {
        Unknown,
        Texture,
        RenderTexture,
        HistoryRenderTexture,
        RenderList,
        Buffer,
        Shader,
        ComputeShader,
        Fence,
        Color,
        Vector4,
        Matrix4x4,
        Integer,
        Float,
        Memory,

        _Count,
    };

    enum class RenderGraphResourceUsage : zp_uint32_t
    {
        Read = 1,
        Write = 2,
        ReadWrite = 3,
    };

    enum class RenderGraphPassFlags : zp_uint32_t
    {
        None = 0,

        DisablePassCulling = 1 << 0,
        DisablePassRenderListCulling = 1 << 1,
        DisablePassMerging = 1 << 2,
        DisablePassReordering = 1 << 3,

        All = zp_uint32_t( ~0 ),
    };

    template<RenderGraphResourceType T>
    struct RenderGraphHandle
    {
        RenderGraphResourceType type = T;
        zp_uint32_t index = -1;
    };

    template<>
    struct RenderGraphHandle<RenderGraphResourceType::Unknown>
    {
        RenderGraphResourceType type = RenderGraphResourceType::Unknown;
        zp_uint32_t index = -1;

        template<RenderGraphResourceType T>
        RenderGraphHandle<T> as()
        {
            return {
                .type = T,
                .index = index
            };
        }
    };

    typedef RenderGraphHandle<RenderGraphResourceType::Unknown> UnknownHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::Texture> TextureHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::RenderTexture> RenderTextureHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::HistoryRenderTexture> HistoryRenderTextureHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::RenderList> RenderListHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::Buffer> BufferHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::Shader> ShaderHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::ComputeShader> ComputeShaderHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::Fence> FenceHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::Color> ColorHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::Vector4> Vector4Handle;
    typedef RenderGraphHandle<RenderGraphResourceType::Matrix4x4> Matrix4x4Handle;
    typedef RenderGraphHandle<RenderGraphResourceType::Integer> IntegerHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::Float> FloatHandle;
    typedef RenderGraphHandle<RenderGraphResourceType::Memory> MemoryHandle;

    //ZP_STATIC_ASSERT( sizeof( RenderGraphHandle ) == sizeof( zp_uint64_t ) );
    class RenderGraph;

    class RenderGraphExecutionContext
    {
    public:
        zp_float32_t GetFloat( const FloatHandle& handle ) const;

        void SetFloat( const FloatHandle& handle, zp_float32_t value );

        Memory GetMemory( const MemoryHandle& handle ) const;

        void Set( const IntegerHandle& handle, zp_int32_t value );

        zp_int32_t Get( const IntegerHandle& handle ) const;
    };

    class RenderGraphRasterExecutionContext
    {
    public:
        RasterCommandBuffer* RequestCommandBuffer();

        void ExecuteCommandBuffer( RasterCommandBuffer* rasterCommandBuffer );

        void ExecuteAndReleaseCommandBuffer( RasterCommandBuffer* rasterCommandBuffer );

        zp_float32_t GetFloat( const FloatHandle& handle ) const;

        void SetFloat( const FloatHandle& handle, zp_float32_t value );

        RenderList* GetRenderList( const RenderListHandle& handle ) const;

        GraphicsBuffer* GetBuffer( const BufferHandle& handle ) const;

        Memory GetMemory( const MemoryHandle& handle ) const;
    };

    class RenderGraphComputeExecutionContext
    {
    public:
        ComputeCommandBuffer* RequestCommandBuffer();

        void ExecuteCommandBuffer( ComputeCommandBuffer* computeCommandBuffer );

        void ExecuteAndReleaseCommandBuffer( ComputeCommandBuffer* computeCommandBuffer );

        zp_float32_t GetFloat( const FloatHandle& handle ) const;

        void SetFloat( const FloatHandle& handle, zp_float32_t value );

        GraphicsBuffer* GetBuffer( const BufferHandle& handle ) const;

        Memory GetMemory( const MemoryHandle& handle ) const;
    };

    struct CameraData
    {
        Matrix4x4f worldToCamera;
        Matrix4x4f cameraToHClip;
        Vector3f positionWS;
        Vector3f forwardWS;
        int type;
    };

    class CompiledRenderGraphExecutionContext
    {
    public:
    };

    class CompiledRenderGraph
    {
    public:
        void Execute( const CompiledRenderGraphExecutionContext& ctx );
    };

    template<typename T>
    class RenderGraphExecutionBuilder
    {
    public:
        RenderGraphExecutionBuilder( RenderGraph* renderGraph, zp_size_t index );

        void SetFlags( RenderGraphPassFlags flags );

        MemoryHandle AllocateMemory( MemoryHandle& memoryHandle, zp_size_t size );

        MemoryHandle AllocateMemory( MemoryHandle& memoryHandle, zp_size_t stride, zp_size_t count );

        MemoryHandle AllocateMemoryIndirect( MemoryHandle& memoryHandle, const IntegerHandle& size );

        MemoryHandle AllocateMemoryIndirect( MemoryHandle& memoryHandle, zp_size_t stride, const IntegerHandle& count );

        IntegerHandle UseInteger( IntegerHandle& integerHandle, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

        FloatHandle UseFloat( FloatHandle& floatHandle, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

        TextureHandle UseTexture( TextureHandle& texture, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

        MemoryHandle UseMemory( MemoryHandle& memoryHandle, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

        void SetExecution( const Function<void, RenderGraphExecutionContext&, const T*>& execution );

    private:
        RenderGraph* m_renderGraph;
        zp_size_t m_passIndex;
    };

    template<typename T>
    class RenderGraphRasterBuilder
    {
    public:
        RenderGraphRasterBuilder( RenderGraph* renderGraph, zp_size_t index );

        ~RenderGraphRasterBuilder();

        void SetFlags( RenderGraphPassFlags flags );

        RenderListHandle CreateRenderList( int renderListDesc );

        RenderTextureHandle CreateRenderTexture( int renderTextureDesc );

        MemoryHandle AllocateMemory( zp_size_t size );

        MemoryHandle AllocateMemoryIndirect( IntegerHandle& size );

        void SetColorAttachment( zp_uint32_t index, RenderTextureHandle& colorAttachment, RenderTargetLoadAction loadAction, RenderTargetStoreAction storeAction );

        void SetDepthAttachment( RenderTextureHandle& depthAttachment, RenderTargetLoadAction loadAction, RenderTargetStoreAction storeAction );

        void SetInputAttachment( zp_uint32_t index, RenderTextureHandle& attachment );

        FloatHandle UseInteger( IntegerHandle& integerHandle, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

        FloatHandle UseFloat( FloatHandle& floatHandle, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

        TextureHandle UseTexture( TextureHandle& texture, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

        RenderTextureHandle UseRenderTexture( RenderTextureHandle& renderTexture, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

        MemoryHandle UseMemory( MemoryHandle& memoryHandle, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

        FenceHandle CreateFence();

        void WaitOnFence( const FenceHandle& fence );

        void SetExecution( const Function<void, RenderGraphRasterExecutionContext&, const T*>& execution );

    private:
        RenderGraph* m_renderGraph;
        zp_size_t m_passIndex;
    };

    template<typename T>
    class RenderGraphComputeBuilder
    {
    public:
        RenderGraphComputeBuilder( RenderGraph* renderGraph, zp_size_t index );

        void SetFlags( RenderGraphPassFlags flags );

        MemoryHandle UseMemory( MemoryHandle& memoryHandle, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

        FenceHandle CreateFence();

        void WaitOnFence( const FenceHandle& fence );

        void SetExecution( const Function<void, RenderGraphComputeExecutionContext&, const T*>& execution );

    private:
        RenderGraph* m_renderGraph;
        zp_size_t m_passIndex;
    };

    class RenderGraph
    {
    public:
        RenderGraph( Memory scratchMemory, MemoryLabel memoryLabel );

        void BeginRecording();

        RenderTextureHandle ImportBackBufferColor();

        RenderTextureHandle ImportBackBufferDepth();

        RenderTextureHandle CreateRenderTexture( int renderTextureDesc );

        FloatHandle CreateGlobalFloat( zp_float32_t value );

        template<typename T>
        RenderGraphExecutionBuilder<T> AddExecutionPass( T** outPassData )
        {
            void* mem;
            RequestPassData( sizeof( T ), &mem );
            *outPassData = static_cast<T*>( mem );

            const zp_size_t passIndex = RequestExecutionPass();
            m_executionPasses[ passIndex ].passData = { .ptr = mem, .size = sizeof( T ) };
            return { this, passIndex };
        }

        template<typename T>
        RenderGraphRasterBuilder<T> AddRasterPass( T** outPassData )
        {
            void* mem;
            RequestPassData( sizeof( T ), &mem );
            *outPassData = static_cast<T*>( mem );

            const zp_size_t passIndex = RequestRasterPass();
            m_rasterPasses[ passIndex ].passData = { .ptr = mem, .size = sizeof( T ) };
            return { this, passIndex };
        }

        template<typename T>
        RenderGraphComputeBuilder<T> AddComputePass( T** outPassData )
        {
            void* mem;
            RequestPassData( sizeof( T ), &mem );
            *outPassData = static_cast<T*>( mem );

            const zp_size_t passIndex = RequestComputePass();
            m_computePasses[ passIndex ].passData = { .ptr = mem, .size = sizeof( T ) };
            return { this, passIndex };
        }

        void Present( const RenderTextureHandle& present );

        void EndRecording();

        void Compile( CompiledRenderGraph& compiledRenderGraph ) const;

        Memory AllocateFrameMemory( zp_size_t size );

        template<typename T>
        Memory AllocateFrameMemoryArray( zp_size_t length )
        {
            return AllocateFrameMemory( sizeof( T ) * length );
        }

    private:
        zp_size_t RequestExecutionPass();

        zp_size_t RequestRasterPass();

        zp_size_t RequestComputePass();

        void RequestPassData( zp_size_t passDataSize, void** outPassData );

        struct ExecutionPass
        {
            FixedArray<UnknownHandle, 16> inputs;
            FixedArray<UnknownHandle, 16> outputs;
            zp_size_t compileTimeConditionalFunctionIndex;
            zp_size_t runtimeConditionalFunctionIndex;
            zp_size_t executionFunctionIndex;
            Memory passData;
            RenderGraphPassFlags flags;
        };

        struct RasterPass
        {
            FixedArray<UnknownHandle, 16> inputs;
            FixedArray<UnknownHandle, 16> outputs;
            zp_size_t compileTimeConditionalFunctionIndex;
            zp_size_t runtimeConditionalFunctionIndex;
            zp_size_t executionFunctionIndex;
            Memory passData;
            RenderGraphPassFlags flags;
        };

        struct ComputePass
        {
            FixedArray<UnknownHandle, 16> inputs;
            FixedArray<UnknownHandle, 16> outputs;
            zp_size_t compileTimeConditionalFunctionIndex;
            zp_size_t runtimeConditionalFunctionIndex;
            zp_size_t executionFunctionIndex;
            Memory passData;
            RenderGraphPassFlags flags;
        };

        struct ResolvePass
        {
            FixedArray<UnknownHandle, 16> inputs;
            FixedArray<UnknownHandle, 16> outputs;
            zp_size_t compileTimeConditionalFunctionIndex;
            zp_size_t runtimeConditionalFunctionIndex;
            zp_size_t executionFunctionIndex;
            Memory passData;
            RenderGraphPassFlags flags;
        };

        MemoryAllocator<FixedAllocatedMemoryStorage, LinearAllocatorPolicy, NullMemoryLock> m_frameAllocator;
        MemoryList<ExecutionPass> m_executionPasses;
        MemoryList<RasterPass> m_rasterPasses;
        MemoryList<ComputePass> m_computePasses;
        MemoryList<Function<void, RenderGraphExecutionContext&, const void*>> m_executions;
        MemoryList<Function<zp_bool_t, RenderGraphExecutionContext&, const void*>> m_conditionals;

        //MemoryList<int> m_resourceUnknown;
        MemoryList<int> m_resourceTexture;
        MemoryList<int> m_resourceRenderTexture;
        MemoryList<int> m_resourceHistoryRenderTexture;
        MemoryList<RenderList> m_resourceRenderList;
        MemoryList<int> m_resourceBuffer;
        MemoryList<int> m_resourceShader;
        MemoryList<int> m_resourceComputeShader;
        MemoryList<int> m_resourceFence;
        MemoryList<Color> m_resourceColor;
        MemoryList<Vector4f> m_resourceVector4;
        MemoryList<Matrix4x4f> m_resourceMatrix4x4;
        MemoryList<zp_int32_t> m_resourceInteger;
        MemoryList<zp_float32_t> m_resourceFloat;
        MemoryList<Memory> m_resourceMemory;

        zp_size_t m_resourceCounts[(zp_size_t)RenderGraphResourceType::_Count];
        zp_size_t m_executionPassCount;
        zp_size_t m_rasterPassCount;
        zp_size_t m_computePassCount;

    public:
        const MemoryLabel memoryLabel;
    };
}

namespace zp
{
    template<typename T>
    RenderGraphRasterBuilder<T>::RenderGraphRasterBuilder( RenderGraph* renderGraph, zp_size_t index )
        : m_renderGraph( renderGraph )
        , m_passIndex( index )
    {
    }

    template<typename T>
    RenderGraphRasterBuilder<T>::~RenderGraphRasterBuilder()
    {
    }

    template<typename T>
    void RenderGraphRasterBuilder<T>::SetFlags( RenderGraphPassFlags flags )
    {

    }

    template<typename T>
    RenderListHandle RenderGraphRasterBuilder<T>::CreateRenderList( int renderListDesc )
    {
        return zp::RenderListHandle();
    }

    template<typename T>
    RenderTextureHandle RenderGraphRasterBuilder<T>::CreateRenderTexture( int renderTextureDesc )
    {
        return zp::RenderTextureHandle();
    }

    template<typename T>
    MemoryHandle RenderGraphRasterBuilder<T>::AllocateMemory( zp_size_t size )
    {
        return zp::MemoryHandle();
    }

    template<typename T>
    MemoryHandle RenderGraphRasterBuilder<T>::AllocateMemoryIndirect( IntegerHandle& size )
    {
        return zp::MemoryHandle();
    }

    template<typename T>
    void RenderGraphRasterBuilder<T>::SetColorAttachment( zp_uint32_t index, RenderTextureHandle& colorAttachment, RenderTargetLoadAction loadAction, RenderTargetStoreAction storeAction )
    {

    }

    template<typename T>
    void RenderGraphRasterBuilder<T>::SetDepthAttachment( RenderTextureHandle& depthAttachment, RenderTargetLoadAction loadAction, RenderTargetStoreAction storeAction )
    {

    }

    template<typename T>
    void RenderGraphRasterBuilder<T>::SetInputAttachment( zp_uint32_t index, RenderTextureHandle& attachment )
    {

    }

    template<typename T>
    FloatHandle RenderGraphRasterBuilder<T>::UseInteger( IntegerHandle& integerHandle, RenderGraphResourceUsage usage )
    {
        return zp::FloatHandle();
    }

    template<typename T>
    FloatHandle RenderGraphRasterBuilder<T>::UseFloat( FloatHandle& floatHandle, RenderGraphResourceUsage usage )
    {
        return zp::FloatHandle();
    }

    template<typename T>
    TextureHandle RenderGraphRasterBuilder<T>::UseTexture( TextureHandle& texture, RenderGraphResourceUsage usage )
    {
        return zp::TextureHandle();
    }

    template<typename T>
    RenderTextureHandle RenderGraphRasterBuilder<T>::UseRenderTexture( RenderTextureHandle& renderTexture, RenderGraphResourceUsage usage )
    {
        return zp::RenderTextureHandle();
    }

    template<typename T>
    MemoryHandle RenderGraphRasterBuilder<T>::UseMemory( MemoryHandle& memoryHandle, RenderGraphResourceUsage usage )
    {
        return zp::MemoryHandle();
    }

    template<typename T>
    FenceHandle RenderGraphRasterBuilder<T>::CreateFence()
    {
        return zp::FenceHandle();
    }

    template<typename T>
    void RenderGraphRasterBuilder<T>::WaitOnFence( const FenceHandle& fence )
    {

    }

    template<typename T>
    void RenderGraphRasterBuilder<T>::SetExecution( const Function<void, RenderGraphRasterExecutionContext&, const T*>& execution )
    {
        Function<void, RenderGraphRasterExecutionContext&, const void*> exec;

    }
}
#if 0
namespace zp
{
    //
    //
    //

    struct PassData
    {
        RenderTextureHandle a;
        RenderListHandle l;
        FloatHandle b;
        BufferHandle buff;
        MemoryHandle mem;
        IntegerHandle lc;

        Vector3f data;
        int other;
    };

    struct LightData
    {
        Color tint;
        zp_uint32_t temperature;
        zp_uint32_t lux;
        int type;
    };

    void ahaha()
    {
        AllocMemory scratch( MemoryLabels::Graphics, 1 MB );

        // Build graph
        TextureHandle f;
        RenderGraph g( scratch.memory(), MemoryLabels::Graphics );

        g.BeginRecording();

        FloatHandle fh = g.CreateGlobalFloat( 10 );
        RenderTextureHandle rt = g.CreateRenderTexture( 0 );
        RenderTextureHandle bbc = g.ImportBackBufferColor();
        RenderTextureHandle bbd = g.ImportBackBufferDepth();
        MemoryHandle mh;
        IntegerHandle lightCount;

        {
            PassData* pp;
            auto builder = g.AddExecutionPass<PassData>( &pp );
            builder.SetFlags( RenderGraphPassFlags::DisablePassCulling );
            pp->b = builder.UseFloat( fh, RenderGraphResourceUsage::Write );
            pp->mem = builder.AllocateMemoryIndirect( mh, sizeof( LightData ), lightCount ); // implicit read for lightCount and implicit write for mh/mem
            pp->lc = builder.UseInteger( lightCount, RenderGraphResourceUsage::Write ); // adds write to lightCount

            const auto exe = Function<void, RenderGraphExecutionContext&, const PassData*>::from_function( []( RenderGraphExecutionContext& ctx, const PassData* passData )
            {
                ctx.SetFloat( passData->b, 10 );

                ctx.Set( passData->lc, 10 ); // sets lightCount, required before memory is allocated

                Memory mem = ctx.GetMemory( passData->mem ); // resolves lightCount resource and allocates memory
                zp_memset( mem.ptr, mem.size, 0 );

                ctx.Set( passData->lc, 20 ); // sets lightCount, but does not change memory since it was already called
                ctx.GetMemory( passData->mem ); // returns already allocated memory, does not change even though lightCount changed

            } );
            builder.SetExecution( exe );
        }

        {
            PassData* pp;
            auto builder = g.AddRasterPass<PassData>( &pp );
            builder.SetColorAttachment( 0, rt, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store );
            builder.SetDepthAttachment( bbd, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store );
            pp->b = builder.UseFloat( fh, RenderGraphResourceUsage::Write );
            pp->l = builder.CreateRenderList( 0 );
            pp->mem = builder.UseMemory( mh, RenderGraphResourceUsage::ReadWrite );

            Function<void, RenderGraphRasterExecutionContext&, const PassData*> exe( []( RenderGraphRasterExecutionContext& ctx, const PassData* passData )
            {
                ctx.SetFloat( passData->b, passData->data.x );

                RasterCommandBuffer* cmd = ctx.RequestCommandBuffer();

                cmd->BindBuffer( 0, ctx.GetBuffer( passData->buff ) );

                cmd->DrawRenderList( ctx.GetRenderList( passData->l ) );

                ctx.ExecuteAndReleaseCommandBuffer( cmd );

            } );
            builder.SetExecution( exe );
        }

        {
            PassData* pp;
            auto builder = g.AddComputePass<PassData>( &pp );
            pp->mem = builder.UseMemory( mh );

            auto exe = Function<void, RenderGraphComputeExecutionContext&, const PassData*>::from_function( []( RenderGraphComputeExecutionContext& ctx, const PassData* passData )
            {
                ctx.SetFloat( passData->b, passData->data.x );

                ComputeCommandBuffer* cmd = ctx.RequestCommandBuffer();

                Memory mem = ctx.GetMemory( passData->mem );

                cmd->SetBufferData( ctx.GetBuffer( passData->buff ), mem );

                cmd->BindBuffer( 0, ctx.GetBuffer( passData->buff ) );
                cmd->Dispatch( 0, 0, { .width = 8, .height = 8, .depth = 1 } );

                ctx.ExecuteAndReleaseCommandBuffer( cmd );

            } );
            builder.SetExecution( exe );
        }

        {
            PassData* pp;
            auto builder = g.AddRasterPass<PassData>( &pp );
            builder.SetColorAttachment( 0, bbc, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store );
            builder.SetDepthAttachment( bbd, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store );
            builder.SetInputAttachment( 0, rt );

            pp->b = builder.UseFloat( fh, RenderGraphResourceUsage::Read );

            auto exe = Function<void, RenderGraphRasterExecutionContext&, const PassData*>::from_function( []( RenderGraphRasterExecutionContext& ctx, const PassData* passData )
            {
                zp_float32_t p = ctx.GetFloat( passData->b );

                RasterCommandBuffer* cmd = ctx.RequestCommandBuffer();

                cmd->DrawProcedural();

                ctx.ExecuteAndReleaseCommandBuffer( cmd );

            } );
            builder.SetExecution( exe );
        }

        // Add final present node to build the tree from
        g.Present( bbc );

        g.EndRecording();

        // Compile
        CompiledRenderGraph cg;

        g.Compile( cg );

        // Execute per camera
        const int cameraCount = 1;
        for( int c = 0; c < cameraCount; ++c )
        {
            CompiledRenderGraphExecutionContext ctx;
            cg.Execute( ctx );
        }
    }
}
#endif

#endif //ZP_RENDERGRAPH_H
