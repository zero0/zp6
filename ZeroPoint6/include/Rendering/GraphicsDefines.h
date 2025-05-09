//
// Created by phosg on 2/21/2022.
//

#ifndef ZP_GRAPHICSDEFINES_H
#define ZP_GRAPHICSDEFINES_H

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
        ZP_DISPLAY_FORMAT_D24_UNORM_UINT,
        ZP_DISPLAY_FORMAT_D24_UNORM_S8_UINT,
        ZP_DISPLAY_FORMAT_D32_FLOAT,
        ZP_DISPLAY_FORMAT_D32_FLOAT_S8_UINT,

        DisplayFormat_Count
    };

    enum GraphicsFormat
    {
        ZP_GRAPHICS_FORMAT_UNKNOWN,

        // 8
        ZP_GRAPHICS_FORMAT_R8_UNORM,
        ZP_GRAPHICS_FORMAT_R8_SNORM,
        ZP_GRAPHICS_FORMAT_R8_UINT,
        ZP_GRAPHICS_FORMAT_R8_SINT,
        ZP_GRAPHICS_FORMAT_R8_SRGB,

        ZP_GRAPHICS_FORMAT_R8G8_UNORM,
        ZP_GRAPHICS_FORMAT_R8G8_SNORM,
        ZP_GRAPHICS_FORMAT_R8G8_UINT,
        ZP_GRAPHICS_FORMAT_R8G8_SINT,
        ZP_GRAPHICS_FORMAT_R8G8_SRGB,

        ZP_GRAPHICS_FORMAT_R8G8B8_UNORM,
        ZP_GRAPHICS_FORMAT_R8G8B8_SNORM,
        ZP_GRAPHICS_FORMAT_R8G8B8_UINT,
        ZP_GRAPHICS_FORMAT_R8G8B8_SINT,
        ZP_GRAPHICS_FORMAT_R8G8B8_SRGB,

        ZP_GRAPHICS_FORMAT_R8G8B8A8_UNORM,
        ZP_GRAPHICS_FORMAT_R8G8B8A8_SNORM,
        ZP_GRAPHICS_FORMAT_R8G8B8A8_UINT,
        ZP_GRAPHICS_FORMAT_R8G8B8A8_SINT,
        ZP_GRAPHICS_FORMAT_R8G8B8A8_SRGB,

        // 16
        ZP_GRAPHICS_FORMAT_R16_UNORM,
        ZP_GRAPHICS_FORMAT_R16_SNORM,
        ZP_GRAPHICS_FORMAT_R16_UINT,
        ZP_GRAPHICS_FORMAT_R16_SINT,
        ZP_GRAPHICS_FORMAT_R16_SFLOAT,

        ZP_GRAPHICS_FORMAT_R16G16_UNORM,
        ZP_GRAPHICS_FORMAT_R16G16_SNORM,
        ZP_GRAPHICS_FORMAT_R16G16_UINT,
        ZP_GRAPHICS_FORMAT_R16G16_SINT,
        ZP_GRAPHICS_FORMAT_R16G16_SFLOAT,

        ZP_GRAPHICS_FORMAT_R16G16B16_UNORM,
        ZP_GRAPHICS_FORMAT_R16G16B16_SNORM,
        ZP_GRAPHICS_FORMAT_R16G16B16_UINT,
        ZP_GRAPHICS_FORMAT_R16G16B16_SINT,
        ZP_GRAPHICS_FORMAT_R16G16B16_SFLOAT,

        ZP_GRAPHICS_FORMAT_R16G16B16A16_UNORM,
        ZP_GRAPHICS_FORMAT_R16G16B16A16_SNORM,
        ZP_GRAPHICS_FORMAT_R16G16B16A16_UINT,
        ZP_GRAPHICS_FORMAT_R16G16B16A16_SINT,
        ZP_GRAPHICS_FORMAT_R16G16B16A16_SFLOAT,

        // 32
        ZP_GRAPHICS_FORMAT_R32_UINT,
        ZP_GRAPHICS_FORMAT_R32_SINT,
        ZP_GRAPHICS_FORMAT_R32_SFLOAT,

        ZP_GRAPHICS_FORMAT_R32G32_UINT,
        ZP_GRAPHICS_FORMAT_R32G32_SINT,
        ZP_GRAPHICS_FORMAT_R32G32_SFLOAT,

        ZP_GRAPHICS_FORMAT_R32G32B32_UINT,
        ZP_GRAPHICS_FORMAT_R32G32B32_SINT,
        ZP_GRAPHICS_FORMAT_R32G32B32_SFLOAT,

        ZP_GRAPHICS_FORMAT_R32G32B32A32_UINT,
        ZP_GRAPHICS_FORMAT_R32G32B32A32_SINT,
        ZP_GRAPHICS_FORMAT_R32G32B32A32_SFLOAT,

        // 10
        ZP_GRAPHICS_FORMAT_A2R10G10B10_UNORM,
        ZP_GRAPHICS_FORMAT_A2R10G10B10_SNORM,
        ZP_GRAPHICS_FORMAT_A2R10G10B10_UINT,
        ZP_GRAPHICS_FORMAT_A2R10G10B10_SINT,

        // Depth / Stencil
        ZP_GRAPHICS_FORMAT_D16_UNORM,
        ZP_GRAPHICS_FORMAT_X8D24_UNORM,
        ZP_GRAPHICS_FORMAT_D32_FLOAT,
        ZP_GRAPHICS_FORMAT_S8_UINT,
        ZP_GRAPHICS_FORMAT_D16_UNORM_S8_UINT,
        ZP_GRAPHICS_FORMAT_D24_UNORM_S8_UINT,
        ZP_GRAPHICS_FORMAT_D32_SFLOAT_S8_UINT,

        // Compressed
        ZP_GRAPHICS_FORMAT_BC1_RGB_UNORM,
        ZP_GRAPHICS_FORMAT_BC1_RGB_SRGB,
        ZP_GRAPHICS_FORMAT_BC1_RGBA_UNORM,
        ZP_GRAPHICS_FORMAT_BC1_RGBA_SRGB,

        ZP_GRAPHICS_FORMAT_BC2_RGBA_UNORM,
        ZP_GRAPHICS_FORMAT_BC2_RGBA_SRGB,

        ZP_GRAPHICS_FORMAT_BC3_RGBA_UNORM,
        ZP_GRAPHICS_FORMAT_BC3_RGBA_SRGB,

        ZP_GRAPHICS_FORMAT_BC6H_RGBA_UFLOAT,
        ZP_GRAPHICS_FORMAT_BC6H_RGBA_SFLOAT,

        ZP_GRAPHICS_FORMAT_ASTCS_4x4_RGBA_UNORM,
        ZP_GRAPHICS_FORMAT_ASTCS_4x4_RGBA_SRGB,

        GraphicsFormat_Count,
    };

    enum RenderQueue
    {
        ZP_RENDER_QUEUE_GRAPHICS,
        ZP_RENDER_QUEUE_TRANSFER,
        ZP_RENDER_QUEUE_COMPUTE,
        ZP_RENDER_QUEUE_PRESENT,

        RenderQueue_Count
    };

    enum IndexBufferFormat
    {
        ZP_INDEX_BUFFER_FORMAT_UINT16,
        ZP_INDEX_BUFFER_FORMAT_UINT32,
        ZP_INDEX_BUFFER_FORMAT_UINT8,
        ZP_INDEX_BUFFER_FORMAT_NONE,

        IndexBufferFormat_Count
    };

    enum TextureDimension
    {
        ZP_TEXTURE_DIMENSION_TEXTURE_1D,
        ZP_TEXTURE_DIMENSION_TEXTURE_1D_ARRAY,
        ZP_TEXTURE_DIMENSION_TEXTURE_2D,
        ZP_TEXTURE_DIMENSION_TEXTURE_2D_ARRAY,
        ZP_TEXTURE_DIMENSION_TEXTURE_3D,
        ZP_TEXTURE_DIMENSION_TEXTURE_CUBE_MAP,
        ZP_TEXTURE_DIMENSION_TEXTURE_CUBE_MAP_ARRAY,

        TextureDimension_Count
    };

    enum TextureUsage
    {
        ZP_TEXTURE_USAGE_TRANSFER_SRC = 1 << 0,
        ZP_TEXTURE_USAGE_TRANSFER_DST = 1 << 1,
        ZP_TEXTURE_USAGE_SAMPLED = 1 << 2,
        ZP_TEXTURE_USAGE_STORAGE = 1 << 3,
        ZP_TEXTURE_USAGE_COLOR_ATTACHMENT = 1 << 4,
        ZP_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT = 1 << 5,
        ZP_TEXTURE_USAGE_TRANSIENT_ATTACHMENT = 1 << 6,
    };

    enum GraphicsBufferUsage : zp_uint32_t
    {
        ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_SRC = 1 << 0,
        ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_DST = 1 << 1,

        ZP_GRAPHICS_BUFFER_USAGE_VERTEX_BUFFER = 1 << 2,
        ZP_GRAPHICS_BUFFER_USAGE_INDEX_BUFFER = 1 << 3,
        ZP_GRAPHICS_BUFFER_USAGE_INDIRECT_ARGS = 1 << 4,

        ZP_GRAPHICS_BUFFER_USAGE_UNIFORM = 1 << 5,
        ZP_GRAPHICS_BUFFER_USAGE_UNIFORM_TEXEL = 1 << 6,

        ZP_GRAPHICS_BUFFER_USAGE_STORAGE = 1 << 7,
        ZP_GRAPHICS_BUFFER_USAGE_STORAGE_TEXEL = 1 << 8,
    };
    typedef zp_uint32_t GraphicsBufferUsageFlags;

    enum MemoryPropertyFlags
    {
        ZP_MEMORY_PROPERTY_DEVICE_LOCAL = 1 << 0,
        ZP_MEMORY_PROPERTY_HOST_VISIBLE = 1 << 1,
    };

    enum Topology
    {
        ZP_TOPOLOGY_POINT_LIST,
        ZP_TOPOLOGY_LINE_LIST,
        ZP_TOPOLOGY_LINE_STRIP,
        ZP_TOPOLOGY_TRIANGLE_LIST,
        ZP_TOPOLOGY_TRIANGLE_STRIP,
        ZP_TOPOLOGY_TRIANGLE_FAN,
        Topology_Count,
    };

    enum PolygonFillMode
    {
        ZP_POLYGON_FILL_MODE_FILL,
        ZP_POLYGON_FILL_MODE_LINE,
        ZP_POLYGON_FILL_MODE_POINT,
        PolygonFillMode_Count,
    };

    enum CullMode
    {
        ZP_CULL_MODE_NONE,
        ZP_CULL_MODE_FRONT,
        ZP_CULL_MODE_BACK,
        ZP_CULL_MODE_FRONT_AND_BACK,
        CullMode_Count,
    };

    enum FrontFaceMode
    {
        ZP_FRONT_FACE_MODE_CCW,
        ZP_FRONT_FACE_MODE_CW,
        FrontFaceMode_Count,
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
        SampleCount_Count,
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
        CompareOp_Count
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
        StencilOp_Count,
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
        LogicOp_Count,
    };

    struct StencilState
    {
        StencilOp passOp;
        StencilOp failOp;
        StencilOp depthFailOp;
        CompareOp compareOp;
        zp_uint32_t compareMask;
        zp_uint32_t writeMask;
        zp_uint32_t reference;
    };

    struct DepthStencilState
    {
        CompareOp depthCompareOp;
        zp_float32_t minDepthBounds;
        zp_float32_t maxDepthBounds;
        StencilState front;
        StencilState back;
        ZP_BOOL32( depthTestEnabled );
        ZP_BOOL32( depthWriteEnabled );
        ZP_BOOL32( depthBoundsTestEnabled );
        ZP_BOOL32( stencilTestEnabled );
    };

    enum BlendOp
    {
        ZP_BLEND_OP_ADD,
        ZP_BLEND_OP_SUB,
        ZP_BLEND_OP_REVERSE_SUB,
        ZP_BLEND_OP_MIN,
        ZP_BLEND_OP_MAX,
        BlendOp_Count,
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
        BlendFactor_Count,
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

        VertexInputRate_Count,
    };

    enum PipelineBindPoint
    {
        ZP_PIPELINE_BIND_POINT_GRAPHICS,
        ZP_PIPELINE_BIND_POINT_COMPUTE,

        PipelineBindPoint_Count,
    };

    enum FilterMode
    {
        ZP_FILTER_MODE_NEAREST,
        ZP_FILTER_MODE_LINEAR,

        FilterMode_Count
    };

    enum MipmapMode
    {
        ZP_MIPMAP_MODE_NEAREST,
        ZP_MIPMAP_MODE_LINEAR,

        MipmapMode_Count
    };

    enum SamplerAddressMode
    {
        ZP_SAMPLER_ADDRESS_MODE_REPEAT,
        ZP_SAMPLER_ADDRESS_MODE_MIRROR_REPEAT,
        ZP_SAMPLER_ADDRESS_MODE_CLAMP,
        ZP_SAMPLER_ADDRESS_MODE_BORDER,
        ZP_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP,

        SamplerAddressMode_Count
    };

    enum BorderColor
    {
        ZP_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        ZP_BORDER_COLOR_INT_TRANSPARENT_BLACK,
        ZP_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        ZP_BORDER_COLOR_INT_OPAQUE_BLACK,
        ZP_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        ZP_BORDER_COLOR_INT_OPAQUE_WHITE,

        BorderColor_Count
    };

    enum ColorSpace
    {
        ZP_COLOR_SPACE_SRGB_NONLINEAR,
        ZP_COLOR_SPACE_DISPLAY_P3_NONLINEAR,
        ZP_COLOR_SPACE_DISPLAY_P3_LINEAR,
        ZP_COLOR_SPACE_REC_709_NONLINEAR,
        ZP_COLOR_SPACE_REC_709_LINEAR,
        ZP_COLOR_SPACE_REC_2020_LINEAR,

        ColorSpace_Count
    };

    enum AttachmentLoadOp
    {
        ZP_ATTACHMENT_LOAD_OP_LOAD,
        ZP_ATTACHMENT_LOAD_OP_CLEAR,
        ZP_ATTACHMENT_LOAD_OP_DONT_CARE,

        AttachmentLoadOp_Count
    };

    enum AttachmentStoreOp
    {
        ZP_ATTACHMENT_STORE_OP_STORE,
        ZP_ATTACHMENT_STORE_OP_DONT_CARE,

        AttachmentStoreOp_Count
    };

    enum class DelayedDestroyType
    {
        Buffer,
        BufferView,
        Image,
        ImageView,
        FrameBuffer,
        Swapchain,
        Surface,
        Shader,
        RenderPass,
        Sampler,
        Fence,
        Semaphore,
        Pipeline,
        PipelineLayout,
        Memory,
    };

    struct DelayedDestroy
    {
        zp_uint64_t frameIndex;
        zp_handle_t handle;
        zp_handle_t allocator;
        zp_handle_t localDevice;
        zp_handle_t instance;
        zp_size_t order;
        DelayedDestroyType type;
    };
}

#endif //ZP_GRAPHICSDEFINES_H
