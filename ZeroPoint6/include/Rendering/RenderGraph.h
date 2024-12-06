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

        void EnableScissorRect( const Rect2Di& scissorRect );

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

    enum class RenderGraphResourceUsage : zp_uint32_t;

    constexpr zp_bool_t operator!=( RenderGraphResourceUsage lh, zp_uint32_t rh )
    {
        return (zp_uint32_t)lh != rh;
    }

    constexpr zp_bool_t operator==( RenderGraphResourceUsage lh, zp_uint32_t rh )
    {
        return (zp_uint32_t)lh == rh;
    }

    constexpr RenderGraphResourceUsage operator&( RenderGraphResourceUsage lh, RenderGraphResourceUsage rh )
    {
        return (RenderGraphResourceUsage)( (zp_uint32_t)lh & (zp_uint32_t)rh );
    }

    constexpr RenderGraphResourceUsage operator|( RenderGraphResourceUsage lh, RenderGraphResourceUsage rh )
    {
        return (RenderGraphResourceUsage)( (zp_uint32_t)lh | (zp_uint32_t)rh );
    }

    enum class RenderGraphResourceUsage : zp_uint32_t
    {
        Read = 1 << 0,
        Write = 1 << 1,
        Discard = 1 << 2,

        WriteAll = Write | Discard,
        ReadWrite = Read | Write,
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
    class RenderGraphHandle
    {
    public:
        RenderGraphHandle() : m_data( 0 )
        {}

        ~RenderGraphHandle()
        {
            m_data = 0;
        };

        [[nodiscard]] zp_bool_t IsValid() const
        {
            return m_index > 0;
        }

        [[nodiscard]] RenderGraphHandle<T> NewVersion() const
        {
            return { m_index, m_version + 1u };
        }

    private:
        explicit RenderGraphHandle( zp_uint64_t data ) : m_data( data )
        {}

        RenderGraphHandle( zp_uint32_t index, zp_uint32_t version )
            : m_type( T )
            , m_index( index )
            , m_version( version )
        {}

        union
        {
            zp_uint64_t m_data;
            struct
            {
                RenderGraphResourceType m_type = T;
                zp_uint32_t m_index: 24 = 0;
                zp_uint32_t m_version: 8 = 0;
            };
        };

        friend class RenderGraph;
    };

    template<RenderGraphResourceType T0, RenderGraphResourceType T1>
    constexpr zp_bool_t operator==( const RenderGraphHandle<T0>& lh, const RenderGraphHandle<T1>& rh )
    {
        return lh.m_data == rh.m_data;
    }

    template<RenderGraphResourceType T0, RenderGraphResourceType T1>
    constexpr zp_bool_t operator!=( const RenderGraphHandle<T0>& lh, const RenderGraphHandle<T1>& rh )
    {
        return lh.m_data != rh.m_data;
    }

    template<RenderGraphResourceType T0, RenderGraphResourceType T1>
    constexpr zp_int32_t operator<( const RenderGraphHandle<T0>& lh, const RenderGraphHandle<T1>& rh )
    {
        const zp_int32_t cmp = zp_cmp( lh.m_data, rh.m_data );
        return cmp;
    }

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

    ZP_STATIC_ASSERT( sizeof( UnknownHandle ) == sizeof( zp_uint64_t ) );

    class RenderGraph;

    class RenderGraphExecutionContext
    {
    public:
        zp_float32_t GetFloat( const FloatHandle& handle ) const;

        void SetFloat( const FloatHandle& handle, zp_float32_t value );

        Memory GetMemory( const MemoryHandle& handle ) const;

        zp_float32_t& operator[]( const FloatHandle& handle );

        zp_int32_t& operator[]( const IntegerHandle& handle );
    };

    class RenderGraphRasterExecutionContext
    {
    public:
        RasterCommandBuffer* RequestCommandBuffer();

        void ExecuteCommandBuffer( RasterCommandBuffer* rasterCommandBuffer );

        void ExecuteAndReleaseCommandBuffer( RasterCommandBuffer* rasterCommandBuffer );

        zp_float32_t operator[]( const FloatHandle& handle ) const;

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

        ~RenderGraph();

        void BeginRecording();

        RenderTextureHandle ImportBackBufferColor();

        RenderTextureHandle ImportBackBufferDepth();

        RenderTextureHandle ImportPreviousRenderTarget( const RenderTextureHandle& handle );

        RenderTextureHandle CreateRenderTexture( int renderTextureDesc );

        FloatHandle CreateGlobalFloat( zp_float32_t value );

        IntegerHandle CreateGlobalInteger( zp_int32_t value );

        template<typename T>
        class ExecutionBuilder
        {
        public:
            ExecutionBuilder( RenderGraph* renderGraph, zp_size_t index );

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
        ExecutionBuilder<T> AddExecutionPass( T** outPassData )
        {
            const zp_size_t passDataSize = sizeof( T );

            void* mem;
            const zp_size_t passIndex = RequestPassMemory( passDataSize, &mem );
            *outPassData = static_cast<T*>( mem );

            m_allPassData[ passIndex ] = {
                .inputs {},
                .outputs {},
                .colorAttachments {},
                .inputAttachments {},
                .depthAttachment {},
                .compileTimeConditionalFunctionIndex = -1ULL,
                .runtimeConditionalFunctionIndex = -1ULL,
                .executionFunctionIndex = -1ULL,
                .passData = { .ptr = mem, .size = passDataSize },
                .flags = RenderGraphPassFlags::None,
                .type = PassDataType::Execution,
            };
            return { this, passIndex };
        }

        template<typename T>
        class RasterBuilder
        {
        public:
            ~RasterBuilder();

            void SetFlags( RenderGraphPassFlags flags );

            RenderListHandle CreateRenderList( int renderListDesc );

            RenderTextureHandle CreateRenderTexture( int renderTextureDesc );

            MemoryHandle AllocateMemory( zp_size_t size );

            MemoryHandle AllocateMemoryIndirect( const IntegerHandle& size );

            RenderTextureHandle SetColorAttachment( zp_uint32_t index, const RenderTextureHandle& colorAttachment, RenderTargetLoadAction loadAction, RenderTargetStoreAction storeAction, Color clearColor = Color::black,
                                                    const RenderTextureHandle& resolveTarget = {} );

            RenderTextureHandle SetDepthStencilAttachment( const RenderTextureHandle& depthAttachment, RenderTargetLoadAction loadAction, RenderTargetStoreAction storeAction, zp_float32_t clearDepth = 1.0f, zp_uint32_t clearStencil = 0 );

            RenderTextureHandle SetInputAttachment( zp_uint32_t index, const RenderTextureHandle& attachment, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

            FloatHandle UseInteger( const IntegerHandle& integerHandle, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

            FloatHandle UseFloat( const FloatHandle& floatHandle, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

            TextureHandle UseTexture( const TextureHandle& texture, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

            RenderTextureHandle UseRenderTexture( const RenderTextureHandle& renderTexture, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

            MemoryHandle UseMemory( const MemoryHandle& memoryHandle, RenderGraphResourceUsage usage = RenderGraphResourceUsage::Read );

            FenceHandle CreateFence();

            FenceHandle WaitOnFence( const FenceHandle& fence );

            void SetExecution( const Function<void, RenderGraphRasterExecutionContext&, const T*>& execution );

        private:
            RasterBuilder( RenderGraph* renderGraph, zp_size_t index );

            RenderGraph* m_renderGraph;
            zp_size_t m_passIndex;

            friend class RenderGraph;
        };

        template<typename T>
        RasterBuilder<T> AddRasterPass( T** outPassData )
        {
            const zp_size_t passDataSize = sizeof( T );

            void* mem;
            const zp_size_t passIndex = RequestPassMemory( passDataSize, &mem );
            *outPassData = static_cast<T*>( mem );

            m_allPassData[ passIndex ] = {
                .inputs {},
                .outputs {},
                .colorAttachments {},
                .inputAttachments {},
                .depthAttachment {},
                .compileTimeConditionalFunctionIndex = -1ULL,
                .runtimeConditionalFunctionIndex = -1ULL,
                .executionFunctionIndex = -1ULL,
                .passData = { .ptr = mem, .size = passDataSize },
                .flags = RenderGraphPassFlags::None,
                .type = PassDataType::RasterPass,
            };

            return { this, passIndex };
        }

        template<typename T>
        RenderGraphComputeBuilder<T> AddComputePass( T** outPassData )
        {
            const zp_size_t passDataSize = sizeof( T );

            void* mem;
            const zp_size_t passIndex = RequestPassMemory( passDataSize, &mem );
            *outPassData = static_cast<T*>( mem );

            m_allPassData[ passIndex ] = {
                .inputs {},
                .outputs {},
                .colorAttachments {},
                .inputAttachments {},
                .depthAttachment {},
                .compileTimeConditionalFunctionIndex = -1ULL,
                .runtimeConditionalFunctionIndex = -1ULL,
                .executionFunctionIndex = -1ULL,
                .passData = { .ptr = mem, .size = passDataSize },
                .flags = RenderGraphPassFlags::None,
                .type = PassDataType::ComputePass,
            };

            return { this, passIndex };
        }

        RenderTextureHandle Export( const RenderTextureHandle& output );

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
        zp_size_t RequestPassMemory( zp_size_t passDataSize, void** outPassData );

        enum class PassDataType : zp_uint32_t
        {
            Execution,
            RasterPass,
            ComputePass,
            PresentPass,
        };

        struct PassResource
        {
            zp_uint64_t handle;
            RenderGraphResourceUsage usage;
        };

        struct ColorAttachmentResource
        {
            RenderTextureHandle colorHandle;
            RenderTextureHandle resolveTargetHandle;
            RenderTargetLoadAction loadAction;
            RenderTargetStoreAction storeAction;
            Color clearColor;
        };

        struct DepthStencilAttachmentResource
        {
            RenderTextureHandle depthHandle;
            RenderTargetLoadAction loadAction;
            RenderTargetStoreAction storeAction;
            zp_float32_t clearDepth;
            zp_uint32_t clearStencil;
        };

        enum
        {
            kMaxInputResources = 16,
            kMaxOutputResources = 16,
            kMaxTransientResources = 8,
            kMaxColorAttachments = 8,
            kMaxInputAttachments = 8,
        };

        struct PassData
        {
            FixedArray<PassResource, kMaxInputResources> inputs;
            FixedArray<PassResource, kMaxOutputResources> outputs;
            FixedArray<PassResource, kMaxTransientResources> transient;
            FixedArray<ColorAttachmentResource, kMaxColorAttachments> colorAttachments;
            FixedArray<PassResource, kMaxInputAttachments> inputAttachments;
            DepthStencilAttachmentResource depthAttachment;
            zp_size_t compileTimeConditionalFunctionIndex;
            zp_size_t runtimeConditionalFunctionIndex;
            zp_size_t executionFunctionIndex;
            Memory passData;
            RenderGraphPassFlags flags;
            PassDataType type;
        };

        PassData& GetPassData( zp_size_t index );

        MemoryAllocator<FixedAllocatedMemoryStorage, LinearAllocatorPolicy, NullMemoryLock> m_frameAllocator;
        MemoryList<PassData> m_allPassData;
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

        zp_size_t m_resourceCounts[(zp_size_t) RenderGraphResourceType::_Count];
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
    RenderGraph::RasterBuilder<T>::RasterBuilder( RenderGraph* renderGraph, zp_size_t index )
        : m_renderGraph( renderGraph )
        , m_passIndex( index )
    {
    }

    template<typename T>
    RenderGraph::RasterBuilder<T>::~RasterBuilder()
    {
    }

    template<typename T>
    void RenderGraph::RasterBuilder<T>::SetFlags( RenderGraphPassFlags flags )
    {
        zp::RenderGraph::PassData& passData = m_renderGraph->GetPassData( m_passIndex );

        passData.flags = flags;
    }

    template<typename T>
    RenderListHandle RenderGraph::RasterBuilder<T>::CreateRenderList( int renderListDesc )
    {
        return zp::RenderListHandle();
    }

    template<typename T>
    RenderTextureHandle RenderGraph::RasterBuilder<T>::CreateRenderTexture( int renderTextureDesc )
    {
        return zp::RenderTextureHandle();
    }

    template<typename T>
    MemoryHandle RenderGraph::RasterBuilder<T>::AllocateMemory( zp_size_t size )
    {
        return zp::MemoryHandle();
    }

    template<typename T>
    MemoryHandle RenderGraph::RasterBuilder<T>::AllocateMemoryIndirect( const IntegerHandle& size )
    {
        return zp::MemoryHandle();
    }

    template<typename T>
    RenderTextureHandle RenderGraph::RasterBuilder<T>::SetColorAttachment( zp_uint32_t index, const RenderTextureHandle& colorAttachment, RenderTargetLoadAction loadAction, RenderTargetStoreAction storeAction, Color clearColor,
                                                                           const RenderTextureHandle& resolveTargetHandle )
    {
        zp::RenderGraph::PassData& passData = m_renderGraph->GetPassData( m_passIndex );

        passData.colorAttachments[ index ] = {
            .colorHandle = colorAttachment,
            .resolveTargetHandle = resolveTargetHandle,
            .loadAction = loadAction,
            .storeAction = storeAction,
            .clearColor = clearColor,
        };

        const zp_bool_t requiredWrites = loadAction == RenderTargetLoadAction::Clear || storeAction != RenderTargetStoreAction::DontCare;
        return requiredWrites ? colorAttachment.NewVersion() : colorAttachment;
    }

    template<typename T>
    RenderTextureHandle
    RenderGraph::RasterBuilder<T>::SetDepthStencilAttachment( const RenderTextureHandle& depthAttachment, RenderTargetLoadAction loadAction, RenderTargetStoreAction storeAction, zp_float32_t clearDepth, zp_uint32_t clearStencil )
    {
        zp::RenderGraph::PassData& passData = m_renderGraph->GetPassData( m_passIndex );

        passData.depthAttachment = {
            .depthHandle = depthAttachment,
            .loadAction = loadAction,
            .storeAction = storeAction,
            .clearDepth = clearDepth,
            .clearStencil = clearStencil,
        };

        const zp_bool_t requiredWrites = loadAction == RenderTargetLoadAction::Clear || storeAction != RenderTargetStoreAction::DontCare;
        return requiredWrites ? depthAttachment.NewVersion() : depthAttachment;
    }

    template<typename T>
    RenderTextureHandle RenderGraph::RasterBuilder<T>::SetInputAttachment( zp_uint32_t index, const RenderTextureHandle& attachment, RenderGraphResourceUsage usage )
    {
        zp::RenderGraph::PassData& passData = m_renderGraph->GetPassData( m_passIndex );

        passData.inputAttachments[ index ] = { .handle = attachment.m_data, .usage = usage };

        const zp_bool_t requiredWrites = ( usage & RenderGraphResourceUsage::WriteAll ) != 0;
       return requiredWrites ? attachment.NewVersion() : attachment;
    }

    template<typename T>
    FloatHandle RenderGraph::RasterBuilder<T>::UseInteger( const IntegerHandle& integerHandle, RenderGraphResourceUsage usage )
    {
        return zp::FloatHandle();
    }

    template<typename T>
    FloatHandle RenderGraph::RasterBuilder<T>::UseFloat( const FloatHandle& floatHandle, RenderGraphResourceUsage usage )
    {
        return zp::FloatHandle();
    }

    template<typename T>
    TextureHandle RenderGraph::RasterBuilder<T>::UseTexture( const TextureHandle& texture, RenderGraphResourceUsage usage )
    {
        return zp::TextureHandle();
    }

    template<typename T>
    RenderTextureHandle RenderGraph::RasterBuilder<T>::UseRenderTexture( const RenderTextureHandle& renderTexture, RenderGraphResourceUsage usage )
    {
        return zp::RenderTextureHandle();
    }

    template<typename T>
    MemoryHandle RenderGraph::RasterBuilder<T>::UseMemory( const MemoryHandle& memoryHandle, RenderGraphResourceUsage usage )
    {
        return zp::MemoryHandle();
    }

    template<typename T>
    FenceHandle RenderGraph::RasterBuilder<T>::CreateFence()
    {
        return zp::FenceHandle();
    }

    template<typename T>
    FenceHandle RenderGraph::RasterBuilder<T>::WaitOnFence( const FenceHandle& fence )
    {
        return FenceHandle();
    }

    template<typename T>
    void RenderGraph::RasterBuilder<T>::SetExecution( const Function<void, RenderGraphRasterExecutionContext&, const T*>& execution )
    {
        Function<void, RenderGraphRasterExecutionContext&, const void*> exec;

    }
}
#if 1
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
        IntegerHandle lightCount = g.CreateGlobalInteger( 0 );

        {
            PassData* pp;
            auto builder = g.AddExecutionPass<PassData>( &pp );
            builder.SetFlags( RenderGraphPassFlags::DisablePassCulling );

            pp->b = builder.UseFloat( fh, RenderGraphResourceUsage::Write );
            pp->mem = builder.AllocateMemoryIndirect( mh, sizeof( LightData ), lightCount ); // implicit read for lightCount and implicit write for mh/mem
            pp->lc = builder.UseInteger( lightCount, RenderGraphResourceUsage::Write ); // adds write to lightCount

            const auto exe = Function<void, RenderGraphExecutionContext&, const PassData*>::from_function(
                []( RenderGraphExecutionContext& ctx, const PassData* passData )
                {
                    ctx[ passData->b ] = 5.5f;
                    ctx.SetFloat( passData->b, 10 );

                    ctx[ passData->lc ] = 10;
                    //ctx.Set( passData->lc, 10 ); // sets lightCount, required before memory is allocated

                    Memory mem = ctx.GetMemory( passData->mem ); // resolves lightCount resource and allocates memory
                    zp_memset( mem.ptr, mem.size, 0 );

                    //ctx.Set( passData->lc, 20 ); // sets lightCount, but does not change memory since it was already called
                    ctx.GetMemory( passData->mem ); // returns already allocated memory, does not change even though lightCount changed

                } );
            builder.SetExecution( exe );
        }

        {
            PassData* pp;
            auto builder = g.AddRasterPass<PassData>( &pp );
            builder.SetColorAttachment( 0, rt, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store );
            builder.SetDepthStencilAttachment( bbd, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store );
            pp->b = builder.UseFloat( fh, RenderGraphResourceUsage::Write );
            pp->l = builder.CreateRenderList( 0 );
            pp->mem = builder.UseMemory( mh, RenderGraphResourceUsage::ReadWrite );

            Function<void, RenderGraphRasterExecutionContext&, const PassData*> exe(
                []( RenderGraphRasterExecutionContext& ctx, const PassData* passData )
                {
                    RasterCommandBuffer* cmd = ctx.RequestCommandBuffer();

                    cmd->BindBuffer( 0, ctx.GetBuffer( passData->buff ) );

                    cmd->DrawRenderList( ctx.GetRenderList( passData->l ) );

                    cmd->DrawProcedural();

                    ctx.ExecuteAndReleaseCommandBuffer( cmd );

                } );
            builder.SetExecution( exe );
        }

        {
            PassData* pp;
            auto builder = g.AddComputePass<PassData>( &pp );
            pp->mem = builder.UseMemory( mh );

            auto exe = Function<void, RenderGraphComputeExecutionContext&, const PassData*>::from_function(
                []( RenderGraphComputeExecutionContext& ctx, const PassData* passData )
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
            builder.SetDepthStencilAttachment( bbd, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store );
            builder.SetInputAttachment( 0, rt );

            pp->b = builder.UseFloat( fh, RenderGraphResourceUsage::Read );

            auto exe = Function<void, RenderGraphRasterExecutionContext&, const PassData*>::from_function(
                []( RenderGraphRasterExecutionContext& ctx, const PassData* passData )
                {
                    zp_float32_t p = ctx[ passData->b ];

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
