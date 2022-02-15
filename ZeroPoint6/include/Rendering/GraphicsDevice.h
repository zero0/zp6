//
// Created by phosg on 1/30/2022.
//

#ifndef ZP_GRAPHICSDEVICE_H
#define ZP_GRAPHICSDEVICE_H

#include <Core/Version.h>
#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Allocator.h"

#include "Engine/JobSystem.h"

#include "Rendering/RenderPass.h"
#include "Rendering/GraphicsResource.h"
#include "Rendering/Shader.h"

namespace zp
{
    enum class GraphicsAPI
    {
        Vulkan,
        DirectX12,
    };

    enum DisplayFormat : zp_uint32_t
    {
        ZP_DISPLAY_FORMAT_UNKNOWN = 0,

        // R Component
        ZP_DISPLAY_FORMAT_R8_UINT,
        ZP_DISPLAY_FORMAT_R8_SINT,

        ZP_DISPLAY_FORMAT_R16_UINT,
        ZP_DISPLAY_FORMAT_R16_SINT,
        ZP_DISPLAY_FORMAT_R16_FLOAT,

        ZP_DISPLAY_FORMAT_R32_UINT,
        ZP_DISPLAY_FORMAT_R32_SINT,
        ZP_DISPLAY_FORMAT_R32_FLOAT,

        // RG Components
        ZP_DISPLAY_FORMAT_RG8_UINT,
        ZP_DISPLAY_FORMAT_RG8_SINT,

        ZP_DISPLAY_FORMAT_RG16_UINT,
        ZP_DISPLAY_FORMAT_RG16_SINT,
        ZP_DISPLAY_FORMAT_RG16_FLOAT,

        ZP_DISPLAY_FORMAT_RG32_UINT,
        ZP_DISPLAY_FORMAT_RG32_SINT,
        ZP_DISPLAY_FORMAT_RG32_FLOAT,

        // RGB Components
        ZP_DISPLAY_FORMAT_RGB8_UINT,
        ZP_DISPLAY_FORMAT_RGB8_SINT,
        ZP_DISPLAY_FORMAT_SRGB8_UINT,

        ZP_DISPLAY_FORMAT_RGB16_UINT,
        ZP_DISPLAY_FORMAT_RGB16_SINT,
        ZP_DISPLAY_FORMAT_RGB16_FLOAT,

        ZP_DISPLAY_FORMAT_RGB32_UINT,
        ZP_DISPLAY_FORMAT_RGB32_SINT,
        ZP_DISPLAY_FORMAT_RGB32_FLOAT,

        // RGBA Components
        ZP_DISPLAY_FORMAT_RGBA8_UINT,
        ZP_DISPLAY_FORMAT_RGBA8_SINT,
        ZP_DISPLAY_FORMAT_RGBA8_UNORM,
        ZP_DISPLAY_FORMAT_RGBA8_SNORM,
        ZP_DISPLAY_FORMAT_SRGBA8_UINT,

        ZP_DISPLAY_FORMAT_RGBA16_UINT,
        ZP_DISPLAY_FORMAT_RGBA16_SINT,
        ZP_DISPLAY_FORMAT_RGBA16_UNORM,
        ZP_DISPLAY_FORMAT_RGBA16_SNORM,
        ZP_DISPLAY_FORMAT_RGBA16_FLOAT,

        ZP_DISPLAY_FORMAT_RGBA32_UINT,
        ZP_DISPLAY_FORMAT_RGBA32_SINT,
        ZP_DISPLAY_FORMAT_RGBA32_FLOAT,

        // Depth Buffer
        ZP_DISPLAY_FORMAT_D24S8_UNORM_UINT,
        ZP_DISPLAY_FORMAT_D32_FLOAT,

        // Compressed
        ZP_DISPLAY_FORMAT_RGB_BC1,
        ZP_DISPLAY_FORMAT_RGBA_BC1,
        ZP_DISPLAY_FORMAT_RGBA_BC2,
        ZP_DISPLAY_FORMAT_RGBA_BC3,
        ZP_DISPLAY_FORMAT_ATI1N,
        ZP_DISPLAY_FORMAT_ATI2N,

        zpDisplayFormat_Count,
        zpDisplayFormat_Force32 = 1,
    };

    enum GraphicsFormat
    {
        ZP_GRAPHICS_FORMAT_UNKNOWN,
    };

    enum GraphicsDeviceFeatures
    {
        None = 0,

        GeometryShaderSupport = 1 << 0,
        TessellationShaderSupport = 1 << 1,

        All = ~0,
    };

    enum RenderQueue
    {
        ZP_RENDER_QUEUE_GRAPHICS,
        ZP_RENDER_QUEUE_TRANSFER,
        ZP_RENDER_QUEUE_COMPUTE,
        ZP_RENDER_QUEUE_PRESENT,
    };

    enum IndexBufferFormat
    {
        ZP_INDEX_BUFFER_FORMAT_UINT16,
        ZP_INDEX_BUFFER_FORMAT_UINT32,
        ZP_INDEX_BUFFER_FORMAT_UINT8,
        ZP_INDEX_BUFFER_FORMAT_NONE,
    };

    struct GraphicsDeviceEnumerator
    {

    };

    enum GraphicsBufferUsageFlags
    {
        ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_SRC = 1 << 0,
        ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_DEST = 1 << 1,

        ZP_GRAPHICS_BUFFER_USAGE_VERTEX_BUFFER = 1 << 2,
        ZP_GRAPHICS_BUFFER_USAGE_INDEX_BUFFER = 1 << 3,
        ZP_GRAPHICS_BUFFER_USAGE_INDIRECT_ARGS = 1 << 4,

        ZP_GRAPHICS_BUFFER_USAGE_UNIFORM = 1 << 5,
        ZP_GRAPHICS_BUFFER_USAGE_UNIFORM_TEXEL = 1 << 6,

        ZP_GRAPHICS_BUFFER_USAGE_STORAGE = 1 << 7,
        ZP_GRAPHICS_BUFFER_USAGE_STORAGE_TEXEL = 1 << 8,
    };

    enum MemoryPropertyFlags
    {
        ZP_MEMORY_PROPERTY_DEVICE_LOCAL = 1 << 0,
    };


    struct GraphicsBufferDesc
    {
        const char* name;
        zp_size_t size;
        GraphicsBufferUsageFlags graphicsBufferUsageFlags;
        MemoryPropertyFlags memoryPropertyFlags;
    };

    struct GraphicsBuffer
    {
        zp_handle_t buffer;
        zp_handle_t deviceMemory;

        zp_size_t offset;
        zp_size_t size;
        zp_size_t alignment;

        GraphicsBufferUsageFlags usageFlags;
    };

    typedef GraphicsResource<GraphicsBuffer> GraphicsBufferResource;
    typedef GraphicsResourceHandle<GraphicsBuffer> GraphicsBufferResourceHandle;

    struct Viewport
    {
        zp_float32_t x, y;
        zp_float32_t width, height;
        zp_float32_t minDepth, maxDepth;
    };

    struct ScissorRect
    {
        zp_int32_t x, y;
        zp_int32_t width, height;
    };

    enum Topology
    {
        ZP_TOPOLOGY_POINT_LIST,
        ZP_TOPOLOGY_LINE_LIST,
        ZP_TOPOLOGY_LINE_STRIP,
        ZP_TOPOLOGY_TRIANGLE_LIST,
        ZP_TOPOLOGY_TRIANGLE_STRIP,
        ZP_TOPOLOGY_TRIANGLE_FAN,
    };

    enum PolygonFillMode
    {
        ZP_POLYGON_FILL_MODE_FILL,
        ZP_POLYGON_FILL_MODE_LINE,
        ZP_POLYGON_FILL_MODE_POINT,
    };

    enum CullMode
    {
        ZP_CULL_MODE_NONE,
        ZP_CULL_MODE_FRONT,
        ZP_CULL_MODE_BACK,
        ZP_CULL_MODE_FRONT_AND_BACK,
    };

    enum FrontFaceMode
    {
        ZP_FRONT_FACE_MODE_CCW,
        ZP_FRONT_FACE_MODE_CW,
    };

    enum SampleCount
    {
        ZP_SAMPLE_COUNT_1,
        ZP_SAMPLE_COUNT_2,
        ZP_SAMPLE_COUNT_4,
        ZP_SAMPLE_COUNT_8,
        ZP_SAMPLE_COUNT_16,
        ZP_SAMPLE_COUNT_32,
        ZP_SAMPLE_COUNT_64,
    };

    enum CompareOp
    {
        ZP_COMPARE_OP_NEVER,
        ZP_COMPARE_OP_EQUAL,
        ZP_COMPARE_OP_NOT_EQUAL,
        ZP_COMPARE_OP_LESS,
        ZP_COMPARE_OP_LESS_OR_EQUAL,
        ZP_COMPARE_OP_GREATER,
        ZP_COMPARE_OP_GREATER_OR_EQUAL,
        ZP_COMPARE_OP_ALWAYS,
    };

    enum StencilOp
    {
        ZP_STENCIL_OP_KEEP,
        ZP_STENCIL_OP_ZERO,
        ZP_STENCIL_OP_REPLACE,
        ZP_STENCIL_OP_INC_CLAMP,
        ZP_STENCIL_OP_DEC_CLAMP,
        ZP_STENCIL_OP_INVERT,
        ZP_STENCIL_OP_INC_WRAP,
        ZP_STENCIL_OP_DEC_WRAP,
    };

    enum LogicOp
    {
        ZP_LOGIC_OP_NO_OP,
        ZP_LOGIC_OP_CLEAR,
        ZP_LOGIC_OP_AND,
        ZP_LOGIC_OP_AND_REVERSE,
        ZP_LOGIC_OP_AND_INVERTED,
        ZP_LOGIC_OP_OR,
        ZP_LOGIC_OP_OR_REVERSE,
        ZP_LOGIC_OP_OR_INVERTED,
        ZP_LOGIC_OP_XOR,
        ZP_LOGIC_OP_NOR,
        ZP_LOGIC_OP_EQUAL,
        ZP_LOGIC_OP_COPY,
        ZP_LOGIC_OP_COPY_INVERTED,
        ZP_LOGIC_OP_INVERT,
        ZP_LOGIC_OP_NAND,
        ZP_LOGIC_OP_SET,
    };

    enum BlendOp
    {
        ZP_BLEND_OP_ADD,
        ZP_BLEND_OP_SUB,
        ZP_BLEND_OP_REVERSE_SUB,
        ZP_BLEND_OP_MIN,
        ZP_BLEND_OP_MAX,
    };

    enum BlendFactor
    {
        ZP_BLEND_FACTOR_ZERO,
        ZP_BLEND_FACTOR_ONE,
        ZP_BLEND_FACTOR_SRC_COLOR,
        ZP_BLEND_FACTOR_DST_COLOR,
        ZP_BLEND_FACTOR_SRC_ALPHA,
        ZP_BLEND_FACTOR_DST_ALPHA,
        ZP_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        ZP_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        ZP_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        ZP_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
        ZP_BLEND_FACTOR_CONSTANT_COLOR,
        ZP_BLEND_FACTOR_CONSTANT_ALPHA,
        ZP_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
        ZP_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
        ZP_BLEND_FACTOR_SRC_ALPHA_SATURATE,
        ZP_BLEND_FACTOR_SRC1_COLOR,
        ZP_BLEND_FACTOR_SRC1_ALPHA,
        ZP_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
        ZP_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
    };

    enum ColorComponent
    {
        ZP_COLOR_COMPONENT_R = 1 << 0,
        ZP_COLOR_COMPONENT_G = 1 << 1,
        ZP_COLOR_COMPONENT_B = 1 << 2,
        ZP_COLOR_COMPONENT_A = 1 << 3,

        ZP_COLOR_COMPONENT_RGB = ZP_COLOR_COMPONENT_R | ZP_COLOR_COMPONENT_G | ZP_COLOR_COMPONENT_B,
        ZP_COLOR_COMPONENT_RGBA = ZP_COLOR_COMPONENT_R | ZP_COLOR_COMPONENT_G | ZP_COLOR_COMPONENT_B | ZP_COLOR_COMPONENT_A,
    };

    enum VertexInputRate
    {
        ZP_VERTEX_INPUT_RATE_VERTEX,
        ZP_VERTEX_INPUT_RATE_INSTANCE,
    };

    enum PipelineBindPoint
    {
        ZP_PIPELINE_BIND_POINT_GRAPHICS,
        ZP_PIPELINE_BIND_POINT_COMPUTE,
    };

    struct StencilState
    {
        StencilOp fail;
        StencilOp pass;
        StencilOp depthFail;
        CompareOp compare;
        zp_uint32_t compareMask;
        zp_uint32_t writeMask;
        zp_uint32_t reference;
    };

    struct BlendState
    {
        zp_bool_t blendEnable;
        BlendFactor srcColorBlendFactor;
        BlendFactor dstColorBlendFactor;
        BlendOp colorBlendOp;
        BlendFactor srcAlphaBlendFactor;
        BlendFactor dstAlphaBlendFactor;
        BlendOp alphaBlendOp;
        ColorComponent writeMask;
    };

    struct VertexBinding
    {
        zp_uint32_t binding;
        zp_uint32_t stride;
        VertexInputRate inputRate;
    };

    struct VertexAttribute
    {
        zp_uint32_t location;
        zp_uint32_t binding;
        zp_uint32_t offset;
        GraphicsFormat format;
    };

    struct PipelineLayoutDesc
    {
        const char* name;
    };

    struct PipelineLayout
    {
        zp_handle_t layout;
    };

    typedef GraphicsResource<PipelineLayout> PipelineLayoutResource;
    typedef GraphicsResourceHandle<PipelineLayout> PipelineLayoutResourceHandle;

    struct GraphicsPipelineStateDesc
    {
        const char* name;

        PipelineLayoutResourceHandle layout;

        RenderPassResourceHandle renderPass;
        zp_uint32_t subPass;

        // shader states
        zp_size_t shaderStageCount;
        ShaderResourceHandle shaderStages[ShaderStage_Count];

        // vertex desc
        zp_size_t vertexBindingCount;
        zp_size_t vertexAttributeCount;
        VertexBinding vertexBindings[8];
        VertexAttribute vertexAttributes[8];

        // input assembler
        Topology primitiveTopology;
        zp_bool_t primitiveRestartEnable;

        // tessellation state
        zp_uint32_t patchControlPoints;

        // viewport state
        zp_size_t viewportCount;
        Viewport viewports[16];

        zp_size_t scissorRectCount;
        ScissorRect scissorRects[16];

        // rasterizer state
        zp_bool_t depthClampEnabled;
        zp_bool_t rasterizerDiscardEnable;
        zp_bool_t depthBiasEnable;
        zp_float32_t depthBiasConstantFactor;
        zp_float32_t depthBiasClamp;
        zp_float32_t depthBiasSlopeFactor;
        zp_float32_t lineWidth;
        PolygonFillMode polygonFillMode;
        CullMode cullMode;
        FrontFaceMode frontFaceMode;

        // multisample state
        SampleCount sampleCount;
        zp_float32_t minSampleShading;
        zp_uint64_t sampleMask;
        zp_bool_t sampleShadingEnable;
        zp_bool_t alphaToCoverageEnable;
        zp_bool_t alphaToOneEnable;

        // depth stencil state
        zp_bool_t depthTestEnable;
        zp_bool_t depthWriteEnable;
        zp_bool_t depthBoundsTestEnable;
        zp_float32_t minDepthBounds;
        zp_float32_t maxDepthBounds;
        CompareOp depthCompare;

        zp_bool_t stencilTestEnable;
        StencilState front;
        StencilState back;

        // color blend state
        zp_bool_t blendLogicalOpEnable;
        LogicOp blendLogicalOp;
        zp_float32_t blendConstants[4];
        zp_size_t blendStateCount;
        BlendState blendStates[8];

        // dynamic state

    };


    struct GraphicsPipelineState
    {
        zp_hash128_t pipelineHash;

        zp_handle_t pipelineState;
    };

    struct CommandQueue
    {
        zp_uint64_t frame;
        zp_uint64_t frameIndex;
        zp_handle_t commandBuffer;
        RenderQueue queue;
    };

    class GraphicsDevice;


    GraphicsDevice* CreateGraphicsDevice( MemoryLabel memoryLabel, GraphicsDeviceFeatures graphicsDeviceFeatures );

    void DestroyGraphicsDevice( GraphicsDevice* graphicsDevice );

    ZP_DECLSPEC_NOVTABLE class GraphicsDevice
    {
    ZP_NONCOPYABLE( GraphicsDevice );

    public:
        explicit GraphicsDevice( MemoryLabel memoryLabel );

        virtual ~GraphicsDevice() = 0;

        virtual void createSwapChain( zp_handle_t windowHandle, zp_uint32_t width, zp_uint32_t height, int displayFormat, int colorSpace ) = 0;

        virtual void destroySwapChain() = 0;

        virtual void beginFrame( zp_uint64_t frameIndex ) = 0;

        virtual void submit(zp_uint64_t frameIndex) = 0;

        virtual void present(zp_uint64_t frameIndex) = 0;

        virtual void waitForGPU() = 0;

#pragma region Create & Destroy Resources

        virtual void createRenderPass( const RenderPassDesc* renderPassDesc, RenderPass* renderPass ) = 0;

        virtual void destroyRenderPass( RenderPass* renderPass ) = 0;

        virtual void createGraphicsPipeline( const GraphicsPipelineStateDesc* graphicsPipelineStateDesc, GraphicsPipelineState* graphicsPipelineState ) = 0;

        virtual void destroyGraphicsPipeline( GraphicsPipelineState* graphicsPipelineState ) = 0;

        virtual void createPipelineLayout( const PipelineLayoutDesc* pipelineLayoutDesc, PipelineLayout* pipelineLayout ) = 0;

        virtual void destroyPipelineLayout( PipelineLayout* pipelineLayout ) = 0;

        virtual void createBuffer( const GraphicsBufferDesc* graphicsBufferDesc, GraphicsBuffer* graphicsBuffer ) = 0;

        virtual void destroyBuffer( GraphicsBuffer* graphicsBuffer ) = 0;

        virtual void createShader( const ShaderDesc* shaderDesc, Shader* shader ) = 0;

        virtual void destroyShader( Shader* shader ) = 0;

#pragma endregion

#pragma region Command Queue Operations

        virtual CommandQueue* requestCommandQueue( RenderQueue queue, zp_uint64_t frameIndex ) = 0;

        virtual void beginRenderPass( const RenderPass* renderPass, CommandQueue* commandQueue ) = 0;

        virtual void endRenderPass( CommandQueue* commandQueue ) = 0;

        virtual void bindPipeline( const GraphicsPipelineState* graphicsPipelineState, PipelineBindPoint bindPoint, CommandQueue* commandQueue ) = 0;

        virtual void bindIndexBuffer( const GraphicsBuffer* graphicsBuffer, IndexBufferFormat indexBufferFormat, zp_size_t offset, CommandQueue* commandQueue ) = 0;

        virtual void bindVertexBuffers( zp_uint32_t firstBinding, zp_uint32_t bindingCount, const GraphicsBuffer** graphicsBuffers, zp_size_t* offsets, CommandQueue* commandQueue ) = 0;

#pragma endregion

#pragma region Draw Commands

        virtual void draw( zp_uint32_t vertexCount, zp_uint32_t instanceCount, zp_uint32_t firstVertex, zp_uint32_t firstInstance, CommandQueue* commandQueue ) = 0;

#pragma endregion

#pragma region Profiler Markers

        virtual void beginEventLabel( const char* eventLabel, CommandQueue* commandQueue ) = 0;

        virtual void endEventLabel( CommandQueue* commandQueue ) = 0;

        virtual void markEventLabel( const char* eventLabel, CommandQueue* commandQueue ) = 0;

#pragma endregion

    public:
        MemoryLabel memoryLabel;
    };
}

#endif //ZP_GRAPHICSDEVICE_H
