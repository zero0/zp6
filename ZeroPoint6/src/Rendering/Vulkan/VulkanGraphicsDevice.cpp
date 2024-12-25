//
// Created by phosg on 2/3/2022.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Vector.h"
#include "Core/Set.h"
#include "Core/Math.h"
#include "Core/Profiler.h"
#include "Core/String.h"
#include "Core/Log.h"

#include "Platform/Platform.h"

#include "Engine/MemoryLabels.h"

#include "Rendering/GraphicsDevice.h"
#include "Rendering/Vulkan/VulkanGraphicsDevice.h"

#include "Volk/volk.h"

// ZP_CONCAT(_vkResult_,__LINE__)
#if ZP_DEBUG
#define HR( r )                                             \
do                                                          \
{                                                           \
    if( const VkResult result = (r); result != VK_SUCCESS ) \
    {                                                       \
        Log::error() << #r << Log::endl;      \
        Platform::DebugBreak();                             \
    }                                                       \
}                                                           \
while( false )
#else
#define HR( r )   r
#endif

#define FlagBits( f, z, t, v )         f |= ( (z) & (t) ) ? (v) : 0

namespace zp
{
    namespace
    {

#pragma region Convert Functions

        constexpr VkIndexType Convert( const IndexBufferFormat indexBufferFormat )
        {
            constexpr VkIndexType indexTypeMap[] {
                VK_INDEX_TYPE_UINT16,
                VK_INDEX_TYPE_UINT32,
                VK_INDEX_TYPE_UINT8_EXT,
                VK_INDEX_TYPE_NONE_KHR,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( indexTypeMap ) == IndexBufferFormat_Count );
            return indexTypeMap[ indexBufferFormat ];
        }

        constexpr VkBufferUsageFlags Convert( const GraphicsBufferUsageFlags graphicsBufferUsageFlags )
        {
            VkBufferUsageFlags bufferUsageFlagBits {};

            FlagBits( bufferUsageFlagBits, graphicsBufferUsageFlags, ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_SRC, VK_BUFFER_USAGE_TRANSFER_SRC_BIT );
            FlagBits( bufferUsageFlagBits, graphicsBufferUsageFlags, ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_DEST, VK_BUFFER_USAGE_TRANSFER_DST_BIT );
            FlagBits( bufferUsageFlagBits, graphicsBufferUsageFlags, ZP_GRAPHICS_BUFFER_USAGE_VERTEX_BUFFER, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
            FlagBits( bufferUsageFlagBits, graphicsBufferUsageFlags, ZP_GRAPHICS_BUFFER_USAGE_INDEX_BUFFER, VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
            FlagBits( bufferUsageFlagBits, graphicsBufferUsageFlags, ZP_GRAPHICS_BUFFER_USAGE_INDIRECT_ARGS, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT );
            FlagBits( bufferUsageFlagBits, graphicsBufferUsageFlags, ZP_GRAPHICS_BUFFER_USAGE_UNIFORM, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT );
            FlagBits( bufferUsageFlagBits, graphicsBufferUsageFlags, ZP_GRAPHICS_BUFFER_USAGE_UNIFORM_TEXEL, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT );
            FlagBits( bufferUsageFlagBits, graphicsBufferUsageFlags, ZP_GRAPHICS_BUFFER_USAGE_STORAGE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT );
            FlagBits( bufferUsageFlagBits, graphicsBufferUsageFlags, ZP_GRAPHICS_BUFFER_USAGE_STORAGE_TEXEL, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT );

            return bufferUsageFlagBits;
        }

        constexpr VkMemoryPropertyFlags Convert( MemoryPropertyFlags memoryPropertyFlags )
        {
            VkMemoryPropertyFlags propertyFlags {};

            FlagBits( propertyFlags, memoryPropertyFlags, ZP_MEMORY_PROPERTY_DEVICE_LOCAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
            FlagBits( propertyFlags, memoryPropertyFlags, ZP_MEMORY_PROPERTY_HOST_VISIBLE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

            return propertyFlags;
        }

        constexpr VkPrimitiveTopology Convert( Topology topology )
        {
            constexpr VkPrimitiveTopology primitiveTopologyMap[] {
                VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( primitiveTopologyMap ) == Topology_Count );
            return primitiveTopologyMap[ topology ];
        }

        constexpr VkPolygonMode Convert( PolygonFillMode polygonFillMode )
        {
            constexpr VkPolygonMode polygonModeMap[] {
                VK_POLYGON_MODE_FILL,
                VK_POLYGON_MODE_LINE,
                VK_POLYGON_MODE_POINT,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( polygonModeMap ) == PolygonFillMode_Count );
            return polygonModeMap[ polygonFillMode ];
        }

        constexpr VkCullModeFlags Convert( CullMode cullMode )
        {
            constexpr VkCullModeFlags cullModeFlagsMap[] {
                VK_CULL_MODE_NONE,
                VK_CULL_MODE_FRONT_BIT,
                VK_CULL_MODE_BACK_BIT,
                VK_CULL_MODE_FRONT_AND_BACK,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( cullModeFlagsMap ) == CullMode_Count );
            return cullModeFlagsMap[ cullMode ];
        }

        constexpr VkFrontFace Convert( FrontFaceMode frontFaceMode )
        {
            constexpr VkFrontFace frontFaceMap[] {
                VK_FRONT_FACE_COUNTER_CLOCKWISE,
                VK_FRONT_FACE_CLOCKWISE,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( frontFaceMap ) == FrontFaceMode_Count );
            return frontFaceMap[ frontFaceMode ];
        }

        constexpr VkSampleCountFlagBits Convert( SampleCount sampleCount )
        {
            constexpr VkSampleCountFlagBits sampleCountFlagsMap[] {
                VK_SAMPLE_COUNT_1_BIT,
                VK_SAMPLE_COUNT_2_BIT,
                VK_SAMPLE_COUNT_4_BIT,
                VK_SAMPLE_COUNT_8_BIT,
                VK_SAMPLE_COUNT_16_BIT,
                VK_SAMPLE_COUNT_32_BIT,
                VK_SAMPLE_COUNT_64_BIT,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( sampleCountFlagsMap ) == SampleCount_Count );
            return sampleCountFlagsMap[ sampleCount ];
        }

        constexpr VkCompareOp Convert( CompareOp compareOp )
        {
            constexpr VkCompareOp compareOpMap[] {
                VK_COMPARE_OP_NEVER,
                VK_COMPARE_OP_EQUAL,
                VK_COMPARE_OP_NOT_EQUAL,
                VK_COMPARE_OP_LESS,
                VK_COMPARE_OP_LESS_OR_EQUAL,
                VK_COMPARE_OP_GREATER,
                VK_COMPARE_OP_GREATER_OR_EQUAL,
                VK_COMPARE_OP_ALWAYS,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( compareOpMap ) == CompareOp_Count );
            return compareOpMap[ compareOp ];
        }

        constexpr VkStencilOp Convert( StencilOp stencilOp )
        {
            constexpr VkStencilOp stencilOpMap[] {
                VK_STENCIL_OP_KEEP,
                VK_STENCIL_OP_ZERO,
                VK_STENCIL_OP_REPLACE,
                VK_STENCIL_OP_INCREMENT_AND_CLAMP,
                VK_STENCIL_OP_DECREMENT_AND_CLAMP,
                VK_STENCIL_OP_INVERT,
                VK_STENCIL_OP_INCREMENT_AND_WRAP,
                VK_STENCIL_OP_DECREMENT_AND_WRAP,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( stencilOpMap ) == StencilOp_Count );
            return stencilOpMap[ stencilOp ];
        }

        constexpr VkLogicOp Convert( LogicOp logicOp )
        {
            constexpr VkLogicOp logicOpMap[] {
                VK_LOGIC_OP_NO_OP,
                VK_LOGIC_OP_CLEAR,
                VK_LOGIC_OP_AND,
                VK_LOGIC_OP_AND_REVERSE,
                VK_LOGIC_OP_AND_INVERTED,
                VK_LOGIC_OP_OR,
                VK_LOGIC_OP_OR_REVERSE,
                VK_LOGIC_OP_OR_INVERTED,
                VK_LOGIC_OP_XOR,
                VK_LOGIC_OP_NOR,
                VK_LOGIC_OP_EQUIVALENT,
                VK_LOGIC_OP_COPY,
                VK_LOGIC_OP_COPY_INVERTED,
                VK_LOGIC_OP_INVERT,
                VK_LOGIC_OP_NAND,
                VK_LOGIC_OP_SET,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( logicOpMap ) == LogicOp_Count );
            return logicOpMap[ logicOp ];
        }

        constexpr VkBlendOp Convert( BlendOp blendOp )
        {
            constexpr VkBlendOp blendOpMap[] {
                VK_BLEND_OP_ADD,
                VK_BLEND_OP_SUBTRACT,
                VK_BLEND_OP_REVERSE_SUBTRACT,
                VK_BLEND_OP_MIN,
                VK_BLEND_OP_MAX,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( blendOpMap ) == BlendOp_Count );
            return blendOpMap[ blendOp ];
        }

        constexpr VkBlendFactor Convert( BlendFactor blendFactor )
        {
            constexpr VkBlendFactor blendFactorMap[] {
                VK_BLEND_FACTOR_ZERO,
                VK_BLEND_FACTOR_ONE,
                VK_BLEND_FACTOR_SRC_COLOR,
                VK_BLEND_FACTOR_DST_COLOR,
                VK_BLEND_FACTOR_SRC_ALPHA,
                VK_BLEND_FACTOR_DST_ALPHA,
                VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
                VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
                VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
                VK_BLEND_FACTOR_CONSTANT_COLOR,
                VK_BLEND_FACTOR_CONSTANT_ALPHA,
                VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
                VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
                VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
                VK_BLEND_FACTOR_SRC1_COLOR,
                VK_BLEND_FACTOR_SRC1_ALPHA,
                VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
                VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( blendFactorMap ) == BlendFactor_Count );
            return blendFactorMap[ blendFactor ];
        }

        constexpr VkColorComponentFlags Convert( ColorComponent colorComponent )
        {
            VkColorComponentFlags colorComponentFlags {};

            FlagBits( colorComponentFlags, colorComponent, ZP_COLOR_COMPONENT_R, VK_COLOR_COMPONENT_R_BIT );
            FlagBits( colorComponentFlags, colorComponent, ZP_COLOR_COMPONENT_G, VK_COLOR_COMPONENT_G_BIT );
            FlagBits( colorComponentFlags, colorComponent, ZP_COLOR_COMPONENT_B, VK_COLOR_COMPONENT_B_BIT );
            FlagBits( colorComponentFlags, colorComponent, ZP_COLOR_COMPONENT_A, VK_COLOR_COMPONENT_A_BIT );

            return colorComponentFlags;
        }

        constexpr VkVertexInputRate Convert( VertexInputRate vertexInputRate )
        {
            constexpr VkVertexInputRate vertexInputRateMap[] {
                VK_VERTEX_INPUT_RATE_VERTEX,
                VK_VERTEX_INPUT_RATE_INSTANCE
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( vertexInputRateMap ) == VertexInputRate_Count );
            return vertexInputRateMap[ vertexInputRate ];
        }

        constexpr VkFormat Convert( GraphicsFormat graphicsFormat )
        {
            constexpr VkFormat formatMap[] {
                VK_FORMAT_UNDEFINED,

                // 8
                VK_FORMAT_R8_UNORM,
                VK_FORMAT_R8_SNORM,
                VK_FORMAT_R8_UINT,
                VK_FORMAT_R8_SINT,
                VK_FORMAT_R8_SRGB,

                VK_FORMAT_R8G8_UNORM,
                VK_FORMAT_R8G8_SNORM,
                VK_FORMAT_R8G8_UINT,
                VK_FORMAT_R8G8_SINT,
                VK_FORMAT_R8G8_SRGB,

                VK_FORMAT_R8G8B8_UNORM,
                VK_FORMAT_R8G8B8_SNORM,
                VK_FORMAT_R8G8B8_UINT,
                VK_FORMAT_R8G8B8_SINT,
                VK_FORMAT_R8G8B8_SRGB,

                VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_R8G8B8A8_SNORM,
                VK_FORMAT_R8G8B8A8_UINT,
                VK_FORMAT_R8G8B8A8_SINT,
                VK_FORMAT_R8G8B8A8_SRGB,

                // 16
                VK_FORMAT_R16_UNORM,
                VK_FORMAT_R16_SNORM,
                VK_FORMAT_R16_UINT,
                VK_FORMAT_R16_SINT,
                VK_FORMAT_R16_SFLOAT,

                VK_FORMAT_R16G16_UNORM,
                VK_FORMAT_R16G16_SNORM,
                VK_FORMAT_R16G16_UINT,
                VK_FORMAT_R16G16_SINT,
                VK_FORMAT_R16G16_SFLOAT,

                VK_FORMAT_R16G16B16_UNORM,
                VK_FORMAT_R16G16B16_SNORM,
                VK_FORMAT_R16G16B16_UINT,
                VK_FORMAT_R16G16B16_SINT,
                VK_FORMAT_R16G16B16_SFLOAT,

                VK_FORMAT_R16G16B16A16_UNORM,
                VK_FORMAT_R16G16B16A16_SNORM,
                VK_FORMAT_R16G16B16A16_UINT,
                VK_FORMAT_R16G16B16A16_SINT,
                VK_FORMAT_R16G16B16A16_SFLOAT,

                // 32
                VK_FORMAT_R32_UINT,
                VK_FORMAT_R32_SINT,
                VK_FORMAT_R32_SFLOAT,

                VK_FORMAT_R32G32_UINT,
                VK_FORMAT_R32G32_SINT,
                VK_FORMAT_R32G32_SFLOAT,

                VK_FORMAT_R32G32B32_UINT,
                VK_FORMAT_R32G32B32_SINT,
                VK_FORMAT_R32G32B32_SFLOAT,

                VK_FORMAT_R32G32B32A32_UINT,
                VK_FORMAT_R32G32B32A32_SINT,
                VK_FORMAT_R32G32B32A32_SFLOAT,

                // 10
                VK_FORMAT_A2R10G10B10_UNORM_PACK32,
                VK_FORMAT_A2R10G10B10_SNORM_PACK32,
                VK_FORMAT_A2R10G10B10_UINT_PACK32,
                VK_FORMAT_A2R10G10B10_SINT_PACK32,

                // Depth / Stencil
                VK_FORMAT_D16_UNORM,
                VK_FORMAT_X8_D24_UNORM_PACK32,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
            };
            ZP_ASSERT( static_cast<zp_size_t>( graphicsFormat ) < ZP_ARRAY_SIZE( formatMap ) );

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( formatMap ) == GraphicsFormat_Count );
            return formatMap[ graphicsFormat ];
        }

        constexpr VkShaderStageFlagBits Convert( ShaderStage shaderStage )
        {
            constexpr VkShaderStageFlagBits shaderStageMap[] {
                VK_SHADER_STAGE_VERTEX_BIT,
                VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
                VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                VK_SHADER_STAGE_GEOMETRY_BIT,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                VK_SHADER_STAGE_COMPUTE_BIT,
                VK_SHADER_STAGE_TASK_BIT_EXT,
                VK_SHADER_STAGE_MESH_BIT_EXT,
            };
            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( shaderStageMap ) == ShaderStage_Count );

            return shaderStageMap[ shaderStage ];
        }

        constexpr VkPipelineBindPoint Convert( PipelineBindPoint bindPoint )
        {
            constexpr VkPipelineBindPoint bindPointMap[] {
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                VK_PIPELINE_BIND_POINT_COMPUTE,
            };
            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( bindPointMap ) == PipelineBindPoint_Count );
            return bindPointMap[ bindPoint ];
        }

        constexpr VkImageType Convert( TextureDimension textureDimension )
        {
            constexpr VkImageType imageTypeMap[] {
                VK_IMAGE_TYPE_1D,
                VK_IMAGE_TYPE_1D,
                VK_IMAGE_TYPE_2D,
                VK_IMAGE_TYPE_2D,
                VK_IMAGE_TYPE_3D,
                VK_IMAGE_TYPE_3D,
                VK_IMAGE_TYPE_3D,
            };
            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( imageTypeMap ) == TextureDimension_Count );

            return imageTypeMap[ textureDimension ];
        }

        constexpr VkImageViewType ConvertImageView( TextureDimension textureDimension )
        {
            constexpr VkImageViewType imageViewTypeMap[] {
                VK_IMAGE_VIEW_TYPE_1D,
                VK_IMAGE_VIEW_TYPE_1D_ARRAY,
                VK_IMAGE_VIEW_TYPE_2D,
                VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                VK_IMAGE_VIEW_TYPE_3D,
                VK_IMAGE_VIEW_TYPE_CUBE,
                VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
            };
            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( imageViewTypeMap ) == TextureDimension_Count );

            return imageViewTypeMap[ textureDimension ];
        }

        constexpr zp_bool_t IsTextureArray( TextureDimension textureDimension )
        {
            constexpr zp_bool_t imageTypeMap[] {
                false,
                true,
                false,
                true,
                false,
                false,
                true,
            };
            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( imageTypeMap ) == TextureDimension_Count );

            return imageTypeMap[ textureDimension ];
        }

        constexpr VkFormat Convert( TextureFormat textureFormat )
        {
            constexpr VkFormat formatMap[] {
                VK_FORMAT_UNDEFINED,
            };
            ZP_ASSERT( ZP_ARRAY_SIZE( formatMap ) == TextureFormat_Count );

            return formatMap[ textureFormat ];
        }

        constexpr VkImageUsageFlags Convert( TextureUsage usage )
        {
            VkImageUsageFlags imageUsage {};

            FlagBits( imageUsage, usage, ZP_TEXTURE_USAGE_TRANSFER_SRC, VK_IMAGE_USAGE_TRANSFER_SRC_BIT );
            FlagBits( imageUsage, usage, ZP_TEXTURE_USAGE_TRANSFER_DST, VK_IMAGE_USAGE_TRANSFER_DST_BIT );
            FlagBits( imageUsage, usage, ZP_TEXTURE_USAGE_SAMPLED, VK_IMAGE_USAGE_SAMPLED_BIT );
            FlagBits( imageUsage, usage, ZP_TEXTURE_USAGE_STORAGE, VK_IMAGE_USAGE_STORAGE_BIT );
            FlagBits( imageUsage, usage, ZP_TEXTURE_USAGE_COLOR_ATTACHMENT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT );
            FlagBits( imageUsage, usage, ZP_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT );
            FlagBits( imageUsage, usage, ZP_TEXTURE_USAGE_TRANSIENT_ATTACHMENT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT );

            return imageUsage;
        }

        constexpr VkImageAspectFlags ConvertAspect( TextureUsage textureUsage )
        {
            VkImageAspectFlags aspectFlags {};

            FlagBits( aspectFlags, textureUsage, ZP_TEXTURE_USAGE_COLOR_ATTACHMENT, VK_IMAGE_ASPECT_COLOR_BIT );
            FlagBits( aspectFlags, textureUsage, ZP_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT );

            return aspectFlags;
        }

        constexpr VkFilter Convert( FilterMode filterMode )
        {
            constexpr VkFilter filterMap[] {
                VK_FILTER_NEAREST,
                VK_FILTER_LINEAR,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( filterMap ) == FilterMode_Count );
            return filterMap[ filterMode ];
        }

        constexpr VkSamplerMipmapMode Convert( MipmapMode mipmapMode )
        {
            constexpr VkSamplerMipmapMode mipmapModeMap[] {
                VK_SAMPLER_MIPMAP_MODE_NEAREST,
                VK_SAMPLER_MIPMAP_MODE_LINEAR,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( mipmapModeMap ) == MipmapMode_Count );
            return mipmapModeMap[ mipmapMode ];
        }

        constexpr VkSamplerAddressMode Convert( SamplerAddressMode samplerAddressMode )
        {
            constexpr VkSamplerAddressMode samplerAddressModeMap[] {
                VK_SAMPLER_ADDRESS_MODE_REPEAT,
                VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( samplerAddressModeMap ) == SamplerAddressMode_Count );
            return samplerAddressModeMap[ samplerAddressMode ];
        }

        constexpr VkBorderColor Convert( BorderColor borderColor )
        {
            constexpr VkBorderColor borderColorMap[] {
                VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
                VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
                VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                VK_BORDER_COLOR_INT_OPAQUE_WHITE,
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( borderColorMap ) == BorderColor_Count );
            return borderColorMap[ borderColor ];
        }

        constexpr VkColorSpaceKHR Convert( ColorSpace colorSpace )
        {
            constexpr VkColorSpaceKHR colorSpaceMap[] {
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
                VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,
                VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT,
                VK_COLOR_SPACE_BT709_NONLINEAR_EXT,
                VK_COLOR_SPACE_BT709_LINEAR_EXT,
                VK_COLOR_SPACE_BT2020_LINEAR_EXT
            };

            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( colorSpaceMap ) == ColorSpace_Count );
            return colorSpaceMap[ colorSpace ];
        }

#pragma endregion

#pragma region Create Hash Functions

        constexpr zp_hash128_t CalculateHash( const VkSamplerCreateInfo& createInfo )
        {
            zp_hash128_t hash = zp_fnv128_1a( ZP_NAMEOF( VkSamplerCreateInfo ) );
            hash = zp_fnv128_1a( createInfo.flags, hash );
            hash = zp_fnv128_1a( createInfo.magFilter, hash );
            hash = zp_fnv128_1a( createInfo.minFilter, hash );
            hash = zp_fnv128_1a( createInfo.mipmapMode, hash );
            hash = zp_fnv128_1a( createInfo.addressModeU, hash );
            hash = zp_fnv128_1a( createInfo.addressModeV, hash );
            hash = zp_fnv128_1a( createInfo.addressModeW, hash );
            hash = zp_fnv128_1a( createInfo.mipLodBias, hash );
            hash = zp_fnv128_1a( createInfo.anisotropyEnable, hash );
            hash = zp_fnv128_1a( createInfo.maxAnisotropy, hash );
            hash = zp_fnv128_1a( createInfo.compareEnable, hash );
            hash = zp_fnv128_1a( createInfo.compareOp, hash );
            hash = zp_fnv128_1a( createInfo.minLod, hash );
            hash = zp_fnv128_1a( createInfo.maxLod, hash );
            hash = zp_fnv128_1a( createInfo.borderColor, hash );
            hash = zp_fnv128_1a( createInfo.unnormalizedCoordinates, hash );

            return hash;
        }

        constexpr zp_hash128_t CalculateHash( const VkDescriptorSetLayoutCreateInfo& createInfo )
        {
            zp_hash128_t hash = zp_fnv128_1a( ZP_NAMEOF( VkDescriptorSetLayoutCreateInfo ) );
            hash = zp_fnv128_1a( createInfo.flags, hash );
            hash = zp_fnv128_1a( createInfo.bindingCount, hash );

            for( zp_uint32_t i = 0; i < createInfo.bindingCount; ++i )
            {
                auto binding = createInfo.pBindings + i;
                hash = zp_fnv128_1a( binding->binding, hash );
                hash = zp_fnv128_1a( binding->descriptorType, hash );
                hash = zp_fnv128_1a( binding->descriptorCount, hash );
                hash = zp_fnv128_1a( binding->stageFlags, hash );
            }

            return hash;
        }

        constexpr zp_hash128_t CalculateHash( const VkRenderPassCreateInfo& createInfo )
        {
            zp_hash128_t hash = zp_fnv128_1a( ZP_NAMEOF( VkRenderPassCreateInfo ) );
            hash = zp_fnv128_1a( createInfo.flags, hash );

            hash = zp_fnv128_1a( createInfo.attachmentCount, hash );
            hash = zp_fnv128_1a( createInfo.pAttachments, createInfo.attachmentCount, hash );

            hash = zp_fnv128_1a( createInfo.subpassCount, hash );
            for( zp_uint32_t i = 0; i < createInfo.subpassCount; ++i )
            {
                auto subpass = createInfo.pSubpasses + i;
                hash = zp_fnv128_1a( subpass->flags, hash );
                hash = zp_fnv128_1a( subpass->pipelineBindPoint, hash );

                hash = zp_fnv128_1a( subpass->inputAttachmentCount, hash );
                hash = zp_fnv128_1a( subpass->pInputAttachments, subpass->inputAttachmentCount, hash );

                hash = zp_fnv128_1a( subpass->colorAttachmentCount, hash );
                hash = zp_fnv128_1a( subpass->pColorAttachments, subpass->colorAttachmentCount, hash );
                hash = zp_fnv128_1a( subpass->pResolveAttachments, subpass->colorAttachmentCount, hash );
                hash = zp_fnv128_1a( subpass->pDepthStencilAttachment, subpass->colorAttachmentCount, hash );
            }

            hash = zp_fnv128_1a( createInfo.dependencyCount, hash );
            hash = zp_fnv128_1a( createInfo.pDependencies, createInfo.dependencyCount, hash );

            return hash;
        }

#pragma endregion

#pragma region Debug Callbacks

#if ZP_DEBUG

        template<typename Func, typename ... Args>
        VkResult CallDebugUtilResultEXT( VkInstance instance, const char* name, Args ... args )
        {
            VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;

            auto func = reinterpret_cast<Func>(vkGetInstanceProcAddr( instance, name ));
            if( func )
            {
                result = func( args... );
            }

            return result;
        }

#define CallDebugUtilResult( func, inst, ... )  CallDebugUtilResultEXT<ZP_CONCAT(PFN_, func)>( inst, #func, __VA_ARGS__ )

        template<typename Func, typename ... Args>
        VkResult CallDebugUtilEXT( VkInstance instance, const char* name, Args ... args )
        {
            VkResult result = VK_SUCCESS;

            auto func = reinterpret_cast<Func>(vkGetInstanceProcAddr( instance, name ));
            if( func )
            {
                func( args... );
            }
            else
            {
                result = VK_ERROR_EXTENSION_NOT_PRESENT;
            }

            return result;
        }

#define CallDebugUtil( func, inst, ... )  CallDebugUtilEXT<ZP_CONCAT(PFN_, func)>( inst, #func, __VA_ARGS__ )

        VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData )
        {
            const zp_uint32_t messageMask = 0
                                            #if ZP_DEBUG
                                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                            #endif
                                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

            if( zp_flag32_any_set( messageSeverity, messageMask ) )
            {
                if( zp_flag32_is_set( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ) )
                {
                    Log::info() << pCallbackData->pMessage << Log::endl;
                }
                else if( zp_flag32_is_set( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) )
                {
                    Log::warning() << pCallbackData->pMessage << Log::endl;
                }
                else if( zp_flag32_is_set( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) )
                {
                    Log::error() << pCallbackData->pMessage << Log::endl;
                }

                if( zp_flag32_is_set( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) )
                {
                    const MessageBoxResult result = Platform::ShowMessageBox( nullptr, "Vulkan Error", pCallbackData->pMessage, ZP_MESSAGE_BOX_TYPE_ERROR, ZP_MESSAGE_BOX_BUTTON_ABORT_RETRY_IGNORE );
                    if( result == ZP_MESSAGE_BOX_RESULT_ABORT )
                    {
                        Platform::Exit( 1 );
                    }
                    else if( result == ZP_MESSAGE_BOX_RESULT_RETRY )
                    {
                        Platform::DebugBreak();
                    }
                }
            }

            const VkBool32 shouldAbort = zp_flag32_is_set( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) ? VK_TRUE : VK_FALSE;
            return shouldAbort;
        }

#endif // ZP_DEBUG

#if ZP_DEBUG

        void SetDebugObjectName( VkInstance vkInstance, VkDevice vkDevice, VkObjectType objectType, void* objectHandle, const char* name )
        {
            if( name != nullptr )
            {
                const VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfoExt {
                    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                    .objectType = objectType,
                    .objectHandle = reinterpret_cast<uint64_t>(objectHandle),
                    .pObjectName = name,
                };

                HR( CallDebugUtilResult( vkSetDebugUtilsObjectNameEXT, vkInstance, vkDevice, &debugUtilsObjectNameInfoExt ) );
            }
        }

        template<typename ... Args>
        void SetDebugObjectName( VkInstance vkInstance, VkDevice vkDevice, VkObjectType objectType, void* objectHandle, const char* format, Args... args )
        {
            MutableFixedString512 name;
            name.appendFormat( format, args... );

            SetDebugObjectName( vkInstance, vkDevice, objectType, objectHandle, name.c_str() );
        }

#else // !ZP_DEBUG
#define SetDebugObjectName(...) (void)0
#endif // ZP_DEBUG

#pragma endregion

        enum VenderIDs
        {
            kNVidiaVenderID = 4318,
        };

        void PrintPhysicalDeviceInfo( const VkPhysicalDeviceProperties& properties )
        {
            MutableFixedString512 info;
            info.append( properties.deviceName );
            info.append( ' ' );

            SizeInfo sizeInfo = GetSizeInfoFromBytes( properties.limits.maxMemoryAllocationCount MB );
            info.appendFormat( "%1.1f %s", sizeInfo.size, sizeInfo.mem );
            info.append( ' ' );

            switch( properties.deviceType )
            {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    info.append( "(Other)" );
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    info.append( "(Integrated GPU)" );
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    info.append( "(Discrete)" );
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    info.append( "(Virtual GPU)" );
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    info.append( "(CPU)" );
                    break;
                default:
                    ZP_INVALID_CODE_PATH_MSG( "Unknown DeviceType" );
                    break;
            }
            info.append( ' ' );

            if( properties.vendorID == kNVidiaVenderID ) // NVidia
            {
                info.appendFormat( "%d.%d.%d.%d",
                    ( properties.driverVersion >> 22 ) & 0x03FFU,
                    ( properties.driverVersion >> 14 ) & 0xFFU,
                    ( properties.driverVersion >> 6 ) & 0xFFU,
                    ( properties.driverVersion ) & 0x3FU );
            }
            else
            {
                info.appendFormat( "%x", properties.driverVersion );
            }
            info.append( ' ' );

            info.append( "Vulkan" );
            info.append( ' ' );

            info.appendFormat(
                "%d.%d.%d (%d)",
                VK_API_VERSION_MAJOR( properties.apiVersion ),
                VK_API_VERSION_MINOR( properties.apiVersion ),
                VK_API_VERSION_PATCH( properties.apiVersion ),
                VK_API_VERSION_VARIANT( properties.apiVersion )
            );

            Log::message() << info.c_str() << Log::endl;
        }

        zp_bool_t IsPhysicalDeviceSuitable( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface )
        {
            zp_bool_t isSuitable = true;

            VkPhysicalDeviceProperties2 physicalDeviceProperties {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            };
            vkGetPhysicalDeviceProperties2( physicalDevice, &physicalDeviceProperties );

            Log::message() << "Testing " << physicalDeviceProperties.properties.deviceName << "..." << Log::endl;

            // require discrete gpu
            isSuitable &= physicalDeviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

#if ZP_USE_PROFILER
            isSuitable &= physicalDeviceProperties.properties.limits.timestampComputeAndGraphics;

            //VkPhysicalDeviceFeatures physicalDeviceFeatures {};
            //vkGetPhysicalDeviceFeatures( physicalDevice, &physicalDeviceFeatures );
            //
            //isSuitable &= physicalDeviceFeatures.pipelineStatisticsQuery;
#endif

            // check that surface has formats and present modes
            if( surface != VK_NULL_HANDLE )
            {
                uint32_t formatCount = 0;
                HR( vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, surface, &formatCount, VK_NULL_HANDLE ) );

                uint32_t presentModeCount = 0;
                HR( vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, surface, &presentModeCount, VK_NULL_HANDLE ) );

                isSuitable &= formatCount != 0 && presentModeCount != 0;
            }

            return isSuitable;
        }

#pragma region Swapchain

        VkSurfaceFormatKHR ChooseSwapChainSurfaceFormat( const Vector<VkSurfaceFormatKHR>& supportedSurfaceFormats, const ReadonlyMemoryArray<VkSurfaceFormatKHR>& preferredSurfaceFormats )
        {
            VkSurfaceFormatKHR chosenFormat = supportedSurfaceFormats[ 0 ];

            for( const VkSurfaceFormatKHR& supportedFormat : supportedSurfaceFormats )
            {
                const zp_size_t foundIndex = zp_find_index( preferredSurfaceFormats.begin(), preferredSurfaceFormats.end(), [ &supportedFormat ]( const VkSurfaceFormatKHR& preferredFormat )
                {
                    return supportedFormat.format == preferredFormat.format && supportedFormat.colorSpace == preferredFormat.colorSpace;
                } );

                if( foundIndex != zp::npos )
                {
                    chosenFormat = supportedFormat;
                    break;
                }
            }

            return chosenFormat;
        }

        constexpr VkPresentModeKHR ChooseSwapChainPresentMode( const Vector<VkPresentModeKHR>& presentModes, zp_bool_t vsync )
        {
            VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;

            if( !vsync )
            {
                for( const VkPresentModeKHR& presentMode : presentModes )
                {
                    if( presentMode == VK_PRESENT_MODE_MAILBOX_KHR )
                    {
                        chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                        break;
                    }

                    if( presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR )
                    {
                        chosenPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                    }
                }
            }

            return chosenPresentMode;
        }

        constexpr VkExtent2D ChooseSwapChainExtent( const VkSurfaceCapabilitiesKHR& surfaceCapabilities, const VkExtent2D& requestedExtents )
        {
            VkExtent2D chosenExtents;

            if( surfaceCapabilities.currentExtent.width == zp_limit<zp_uint32_t>::max() && surfaceCapabilities.currentExtent.height == zp_limit<zp_uint32_t>::max() )
            {
                chosenExtents.width = requestedExtents.width;
                chosenExtents.height = requestedExtents.height;
            }
            else
            {
                chosenExtents.width = zp_clamp(
                    requestedExtents.width,
                    surfaceCapabilities.minImageExtent.width,
                    surfaceCapabilities.maxImageExtent.width );
                chosenExtents.height = zp_clamp(
                    requestedExtents.height,
                    surfaceCapabilities.minImageExtent.height,
                    surfaceCapabilities.maxImageExtent.height );
            }

            return chosenExtents;
        }

        struct PipelineStateAccess
        {
            VkPipelineStageFlags2 stage;
            VkAccessFlags2 access;
        };

        PipelineStateAccess MakePipelineStageAccess( VkImageLayout state )
        {
            switch( state )
            {
                case VK_IMAGE_LAYOUT_UNDEFINED:
                    return {
                        .stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                        .access = VK_ACCESS_2_NONE
                    };

                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    return {
                        .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
                    };

                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    return {
                        .stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
                        .access = VK_ACCESS_2_SHADER_READ_BIT
                    };

                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    return {
                        .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                        .access = VK_ACCESS_2_TRANSFER_WRITE_BIT
                    };

                case VK_IMAGE_LAYOUT_GENERAL:
                    return {
                        .stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                        .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT
                    };

                case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                    return {
                        .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .access = VK_ACCESS_2_NONE
                    };

                default:
                    return {
                        .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                        .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT
                    };
            }
        }

        void CmdTransitionImageLayout( VkCommandBuffer cmd, VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageSubresourceRange subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 } )
        {
            const auto [srcStage, srcAccess] = MakePipelineStageAccess( srcLayout );
            const auto [dstStage, dstAccess] = MakePipelineStageAccess( dstLayout );

            const VkImageMemoryBarrier2 imageMemoryBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask        = srcStage,
                .srcAccessMask       = srcAccess,
                .dstStageMask        = dstStage,
                .dstAccessMask       = dstAccess,
                .oldLayout           = srcLayout,
                .newLayout           = dstLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = image,
                .subresourceRange    = subresourceRange,
            };

            const VkDependencyInfo dependencyInfo {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &imageMemoryBarrier,
            };

            vkCmdPipelineBarrier2( cmd, &dependencyInfo );
        }

        VkCommandBuffer RequestSingleUseCommandBuffer( VkDevice device, VkCommandPool cmdPool )
        {
            const VkCommandBufferAllocateInfo allocateInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = cmdPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };

            VkCommandBuffer cmdBuffer {};
            HR( vkAllocateCommandBuffers( device, &allocateInfo, &cmdBuffer ) );

            const VkCommandBufferBeginInfo beginInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };

            HR( vkBeginCommandBuffer( cmdBuffer, &beginInfo ) );

            return cmdBuffer;
        }

        void ReleaseSingleUseCommandBuffer( VkDevice device, VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, const VkAllocationCallbacks* allocationCallbacks )
        {
            HR( vkEndCommandBuffer( cmdBuffer ) );

            const VkFenceCreateInfo fenceCreateInfo {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            };

            VkFence fence {};
            HR( vkCreateFence( device, &fenceCreateInfo, allocationCallbacks, &fence ) );

            const VkCommandBufferSubmitInfo cmdBufferInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = cmdBuffer
            };

            const VkSubmitInfo2 submitInfo {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &cmdBufferInfo
            };

            HR( vkQueueSubmit2( queue, 1, &submitInfo, fence ) );
            HR( vkWaitForFences( device, 1, &fence, VK_TRUE, zp_limit<zp_uint64_t>::max() ) );

            vkDestroyFence( device, fence, allocationCallbacks );
            vkFreeCommandBuffers( device, cmdPool, 1, &cmdBuffer );
        }

#pragma endregion

        constexpr uint32_t FindMemoryTypeIndex( const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties, const zp_uint32_t typeFilter, const VkMemoryPropertyFlags memoryPropertyFlags )
        {
            for( uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++ )
            {
                const VkMemoryPropertyFlags flags = physicalDeviceMemoryProperties.memoryTypes[ i ].propertyFlags;
                if( zp_flag32_is_set( typeFilter, ( 1 << i ) ) && zp_flag32_all_set( flags, memoryPropertyFlags ) )
                {
                    return i;
                }
            }

            ZP_INVALID_CODE_PATH();
            return 0;
        }

#pragma region Custom Allocator Callbacks

        void* AllocationCallback( void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope )
        {
            auto* allocator = static_cast<IMemoryAllocator*>(pUserData);
            return allocator->allocate( size, alignment );
        }

        void* ReallocationCallback( void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope )
        {
            auto* allocator = static_cast<IMemoryAllocator*>(pUserData);
            return allocator->reallocate( pOriginal, size, alignment );
        }

        void FreeCallback( void* pUserData, void* pMemory )
        {
            auto* allocator = static_cast<IMemoryAllocator*>(pUserData);
            allocator->free( pMemory );
        }

#pragma endregion


        struct DelayedDestroyInfo
        {
            zp_uint64_t frameIndex;
            VkInstance vkInstance;
            VkDevice vkLocalDevice;
            const VkAllocationCallbacks* vkAllocatorCallbacks;
            zp_size_t order;
            zp_handle_t vkHandle;
            VkObjectType vkObjectType;
        };

        void ProcessDelayedDestroy( const DelayedDestroyInfo& info )
        {
            switch( info.vkObjectType )
            {
                case VK_OBJECT_TYPE_BUFFER:
                    vkDestroyBuffer( info.vkLocalDevice, static_cast<VkBuffer>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_BUFFER_VIEW:
                    vkDestroyBufferView( info.vkLocalDevice, static_cast<VkBufferView>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_IMAGE:
                    vkDestroyImage( info.vkLocalDevice, static_cast<VkImage>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_IMAGE_VIEW:
                    vkDestroyImageView( info.vkLocalDevice, static_cast<VkImageView>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_FRAMEBUFFER:
                    vkDestroyFramebuffer( info.vkLocalDevice, static_cast<VkFramebuffer>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
                    vkDestroySwapchainKHR( info.vkLocalDevice, static_cast<VkSwapchainKHR>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_SURFACE_KHR:
                    vkDestroySurfaceKHR( info.vkInstance, static_cast<VkSurfaceKHR>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_SHADER_MODULE:
                    vkDestroyShaderModule( info.vkLocalDevice, static_cast<VkShaderModule>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_RENDER_PASS:
                    vkDestroyRenderPass( info.vkLocalDevice, static_cast<VkRenderPass>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_SAMPLER:
                    vkDestroySampler( info.vkLocalDevice, static_cast<VkSampler>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_FENCE:
                    vkDestroyFence( info.vkLocalDevice, static_cast<VkFence>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_SEMAPHORE:
                    vkDestroySemaphore( info.vkLocalDevice, static_cast<VkSemaphore>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_PIPELINE:
                    vkDestroyPipeline( info.vkLocalDevice, static_cast<VkPipeline>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
                    vkDestroyPipelineLayout( info.vkLocalDevice, static_cast<VkPipelineLayout>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                case VK_OBJECT_TYPE_DEVICE_MEMORY:
                    vkFreeMemory( info.vkLocalDevice, static_cast<VkDeviceMemory>(info.vkHandle), info.vkAllocatorCallbacks );
                    break;
                default:
                    ZP_INVALID_CODE_PATH_MSG_ARGS( "Delayed Destroyed not defined: %d", info.vkObjectType );
                    break;
            }
        }

        void DestroyDelayedDestroyHandle( const DelayedDestroy& delayedDestroy )
        {
            auto vkAllocatorCallbacks = static_cast<const VkAllocationCallbacks*>( delayedDestroy.allocator );
            auto vkLocalDevice = static_cast<VkDevice>( delayedDestroy.localDevice );
            auto vkInstance = static_cast<VkInstance>( delayedDestroy.instance );
            switch( delayedDestroy.type )
            {
                case DelayedDestroyType::Buffer:
                    vkDestroyBuffer( vkLocalDevice, static_cast<VkBuffer>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::BufferView:
                    vkDestroyBufferView( vkLocalDevice, static_cast<VkBufferView>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::Image:
                    vkDestroyImage( vkLocalDevice, static_cast<VkImage>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::ImageView:
                    vkDestroyImageView( vkLocalDevice, static_cast<VkImageView>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::FrameBuffer:
                    vkDestroyFramebuffer( vkLocalDevice, static_cast<VkFramebuffer>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::Swapchain:
                    vkDestroySwapchainKHR( vkLocalDevice, static_cast<VkSwapchainKHR>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::Surface:
                    vkDestroySurfaceKHR( vkInstance, static_cast<VkSurfaceKHR>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::Shader:
                    vkDestroyShaderModule( vkLocalDevice, static_cast<VkShaderModule>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::RenderPass:
                    vkDestroyRenderPass( vkLocalDevice, static_cast<VkRenderPass>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::Sampler:
                    vkDestroySampler( vkLocalDevice, static_cast<VkSampler>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::Fence:
                    vkDestroyFence( vkLocalDevice, static_cast<VkFence>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::Semaphore:
                    vkDestroySemaphore( vkLocalDevice, static_cast<VkSemaphore>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::Pipeline:
                    vkDestroyPipeline( vkLocalDevice, static_cast<VkPipeline>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::PipelineLayout:
                    vkDestroyPipelineLayout( vkLocalDevice, static_cast<VkPipelineLayout>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                case DelayedDestroyType::Memory:
                    vkFreeMemory( vkLocalDevice, static_cast<VkDeviceMemory>(delayedDestroy.handle), vkAllocatorCallbacks );
                    break;
                default:
                    ZP_INVALID_CODE_PATH_MSG( "DelayedDestroyType not defined" );
                    break;
            }
        }

        template<typename T0, typename T1>
        constexpr void pNextPushFront( T0& mainInfo, T1& nextInfo )
        {
            nextInfo.pNext = mainInfo.pNext;
            mainInfo.pNext = &nextInfo;
        }

        template<typename T0, typename T1>
        constexpr void pNext( T0& mainInfo, T1& nextInfo )
        {
            mainInfo.pNext = &nextInfo;
        }

        zp_bool_t IsExtensionPropertySupported( const char* extension, const Vector<VkExtensionProperties>& availableExtensions )
        {
            const zp_size_t index = availableExtensions.findIndexOf( [ &extension ]( const VkExtensionProperties& ext ) -> zp_bool_t
            {
                return zp_strcmp( extension, ext.extensionName ) == 0;
            } );

            return index != zp::npos;
        }

        zp_bool_t IsExtensionSupported( const char* extension, const Vector<VkExtensionProperties>& availableExtensions )
        {
            const zp_size_t index = availableExtensions.findIndexOf( [ &extension ]( const VkExtensionProperties& ext ) -> zp_bool_t
            {
                return zp_strcmp( extension, ext.extensionName ) == 0;
            } );

            return index != zp::npos;
        }


#pragma region GetObjectType

        template<typename T>
        constexpr auto GetObjectType( T /*unused*/ ) -> VkObjectType
        {
            return VK_OBJECT_TYPE_UNKNOWN;
        }

        // @formatter:off
#define MAKE_OBJECT_TYPE( vk, ot )                              \
template<>                                                      \
constexpr auto GetObjectType( vk /*unused*/ ) -> VkObjectType   \
{                                                               \
    return ot;                                                  \
}
        // @formatter:on

        MAKE_OBJECT_TYPE( VkInstance, VK_OBJECT_TYPE_INSTANCE );

        MAKE_OBJECT_TYPE( VkPhysicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE );

        MAKE_OBJECT_TYPE( VkDevice, VK_OBJECT_TYPE_DEVICE );

        MAKE_OBJECT_TYPE( VkQueue, VK_OBJECT_TYPE_QUEUE );

        MAKE_OBJECT_TYPE( VkSemaphore, VK_OBJECT_TYPE_SEMAPHORE );

        MAKE_OBJECT_TYPE( VkCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER );

        MAKE_OBJECT_TYPE( VkFence, VK_OBJECT_TYPE_FENCE );

        MAKE_OBJECT_TYPE( VkDeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY );

        MAKE_OBJECT_TYPE( VkBuffer, VK_OBJECT_TYPE_BUFFER );

#undef MAKE_OBJECT_TYPE

#pragma endregion
    }

    namespace
    {

        struct QueueFamilyIndices
        {
            zp_uint32_t graphicsFamily;
            zp_uint32_t transferFamily;
            zp_uint32_t computeFamily;
            zp_uint32_t presentFamily;
        };

        struct QueueInfo
        {
            zp_uint32_t familyIndex;
            zp_uint32_t queueIndex;
            VkQueue vkQueue;
        };

        struct Queues
        {
            QueueInfo graphics;
            QueueInfo transfer;
            QueueInfo compute;
            QueueInfo present;
        };

        class VulkanContext
        {
        public:
            VulkanContext() = default;

            ~VulkanContext();

            void Initialize( const GraphicsDeviceDesc& graphicsDeviceDesc );

            void Destroy();

            void EndFrame( VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence fence );

            template<typename T>
            void QueueDestroy( T handle )
            {
                QueueDestroy( handle, GetObjectType( handle ) );
            }

            void QueueDestroy( void* handle, VkObjectType objectType );


            [[nodiscard]] VkInstance GetInstance() const
            {
                return m_vkInstance;
            }

            [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const
            {
                return m_vkPhysicalDevice;
            }

            [[nodiscard]] VkDevice GetLocalDevice() const
            {
                return m_vkLocalDevice;
            }

            [[nodiscard]] VkSurfaceKHR GetSurface() const
            {
                return m_vkSurface;
            }

            [[nodiscard]] const Queues& GetQueues() const
            {
                return m_queues;
            }

            [[nodiscard]] VkCommandPool GetTransientCommandPool() const
            {
                return m_vkTransientCommandPool;
            }

            [[nodiscard]] VkPipelineCache GetPipelineCache() const
            {
                return m_vkPipelineCache;
            }

            [[nodiscard]] const VkAllocationCallbacks* GetAllocationCallbacks() const
            {
                return &m_vkAllocationCallbacks;
            }

        private:
            VkInstance m_vkInstance;
            VkPhysicalDevice m_vkPhysicalDevice;
            VkDevice m_vkLocalDevice;
            VkSurfaceKHR m_vkSurface;

            VkPipelineCache m_vkPipelineCache;
            VkDescriptorPool m_vkDescriptorPool;
            VkCommandPool m_vkTransientCommandPool;
            FixedArray<VkCommandPool, kBufferedFrameCount> m_vkCommandPools;

#if ZP_DEBUG
            VkDebugUtilsMessengerEXT m_vkDebugMessenger;
#endif
            VkAllocationCallbacks m_vkAllocationCallbacks;

            VkPhysicalDeviceMemoryProperties m_vkPhysicalDeviceMemoryProperties;

            Queues m_queues;

            Vector<DelayedDestroyInfo> m_delayedDestroyed;

            zp_uint64_t m_frameIndex;
        };

        VulkanContext::~VulkanContext()
        {
            ZP_ASSERT( m_vkInstance == VK_NULL_HANDLE );
        }

        void VulkanContext::Initialize( const GraphicsDeviceDesc& graphicsDeviceDesc )
        {
            m_vkAllocationCallbacks = {
                .pUserData = GetAllocator( MemoryLabels::Graphics ),
                .pfnAllocation = AllocationCallback,
                .pfnReallocation = ReallocationCallback,
                .pfnFree = FreeCallback,
            };

            HR( volkInitialize() );

            //
            if( 0 )
            {
                zp_uint32_t count {};
                vkEnumerateInstanceExtensionProperties( nullptr, &count, nullptr );

                Vector<VkExtensionProperties> availableInstanceExtensionProperties( count, MemoryLabels::Temp );
                availableInstanceExtensionProperties.resize_unsafe( count );

                vkEnumerateInstanceExtensionProperties( nullptr, &count, availableInstanceExtensionProperties.data() );

                for( const auto& ext : availableInstanceExtensionProperties )
                {
                    zp_printfln( "%s: %d", ext.extensionName, ext.specVersion );
                }
            }

            // TODO: filter out available extensions with requested extensions
            //Vector<VkExtensionProperties> instanceExtensionProperties( count, MemoryLabels::Temp );

            constexpr const char* kInstanceExtensionNames[] {
                VK_KHR_SURFACE_EXTENSION_NAME,
#if ZP_OS_WINDOWS
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if ZP_DEBUG
                VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
            };

            constexpr const char* kValidationLayers[] {
#if ZP_DEBUG
                "VK_LAYER_KHRONOS_validation"
#endif
            };

            const char* kDeviceExtensions[] {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
                VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
                VK_KHR_MAINTENANCE_6_EXTENSION_NAME,
                VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
                VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
                VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
                VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
            };

            // app info
            const VkApplicationInfo applicationInfo {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = graphicsDeviceDesc.appName.empty() ? "ZeroPoint Application" : graphicsDeviceDesc.appName.c_str(),
                .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
                .pEngineName = "ZeroPoint",
                .engineVersion = VK_MAKE_VERSION( ZP_VERSION_MAJOR, ZP_VERSION_MINOR, ZP_VERSION_PATCH ),
                .apiVersion = VK_API_VERSION_1_3,
            };

            // create instance
            VkInstanceCreateInfo instanceCreateInfo {
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pApplicationInfo = &applicationInfo,
                .enabledLayerCount = ZP_ARRAY_SIZE( kValidationLayers ),
                .ppEnabledLayerNames = kValidationLayers,
                .enabledExtensionCount = ZP_ARRAY_SIZE( kInstanceExtensionNames ),
                .ppEnabledExtensionNames = kInstanceExtensionNames,
            };

#if ZP_DEBUG
            // add debug info to create instance
            VkDebugUtilsMessengerCreateInfoEXT createInstanceDebugMessengerInfo {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = DebugCallback,
                .pUserData = nullptr,
            };

            // TODO: does this need to be here twice?
            pNextPushFront( instanceCreateInfo, createInstanceDebugMessengerInfo );
#endif

            HR( vkCreateInstance( &instanceCreateInfo, &m_vkAllocationCallbacks, &m_vkInstance ) );

            volkLoadInstanceOnly( m_vkInstance );

#if ZP_DEBUG
            // create debug messenger
            const VkDebugUtilsMessengerCreateInfoEXT createDebugMessengerInfo {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = DebugCallback,
                .pUserData = nullptr, // Optional
            };

            HR( CallDebugUtilResult( vkCreateDebugUtilsMessengerEXT, m_vkInstance, m_vkInstance, &createDebugMessengerInfo, &m_vkAllocationCallbacks, &m_vkDebugMessenger ) );
            //HR( CreateDebugUtilsMessengerEXT( m_vkInstance, &createDebugMessengerInfo, nullptr, &m_vkDebugMessenger ));
#endif
            // create surface
            {
#if ZP_OS_WINDOWS
                HWND hWnd = static_cast<HWND>( graphicsDeviceDesc.windowHandle.handle );
                HINSTANCE hInstance = nullptr; //reinterpret_cast<HINSTANCE>(::GetWindowLongPtr( hWnd, GWLP_HINSTANCE ));

                // create surface
                const VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                    .hinstance = hInstance,
                    .hwnd = hWnd,
                };

                HR( vkCreateWin32SurfaceKHR( m_vkInstance, &win32SurfaceCreateInfo, &m_vkAllocationCallbacks, &m_vkSurface ) );
#else
#error "Platform not defined to create VkSurface"
#endif
            }

            // select physical device
            {
                uint32_t physicalDeviceCount = 0;
                HR( vkEnumeratePhysicalDevices( m_vkInstance, &physicalDeviceCount, VK_NULL_HANDLE ) );

                Vector<VkPhysicalDevice> physicalDevices( physicalDeviceCount, MemoryLabels::Temp );
                physicalDevices.resize_unsafe( physicalDeviceCount );

                HR( vkEnumeratePhysicalDevices( m_vkInstance, &physicalDeviceCount, physicalDevices.data() ) );

                for( const VkPhysicalDevice& physicalDevice : physicalDevices )
                {
                    if( IsPhysicalDeviceSuitable( physicalDevice, m_vkSurface ) )
                    {
                        m_vkPhysicalDevice = physicalDevice;
                        break;
                    }
                }

                ZP_ASSERT( m_vkPhysicalDevice );
            }

            // create local device and queue families
            {
                const QueueInfo kDefaultQueueInfo {
                    .familyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .queueIndex = 0,
                    .vkQueue = VK_NULL_HANDLE
                };

                m_queues.graphics = kDefaultQueueInfo;
                m_queues.transfer = kDefaultQueueInfo;
                m_queues.compute = kDefaultQueueInfo;
                m_queues.present = kDefaultQueueInfo;

                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties( m_vkPhysicalDevice, &queueFamilyCount, VK_NULL_HANDLE );

                Vector<VkQueueFamilyProperties> queueFamilyProperties( queueFamilyCount, MemoryLabels::Temp );
                queueFamilyProperties.resize_unsafe( queueFamilyCount );

                vkGetPhysicalDeviceQueueFamilyProperties( m_vkPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data() );

                // find base queries
                for( zp_size_t i = 0; i < queueFamilyCount; ++i )
                {
                    const VkQueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[ i ];
                    if( queueFamilyProperty.queueCount == 0 )
                    {
                        continue;
                    }

                    if( m_queues.graphics.familyIndex == VK_QUEUE_FAMILY_IGNORED &&
                        zp_flag32_is_set( queueFamilyProperty.queueFlags, VK_QUEUE_GRAPHICS_BIT ) )
                    {
                        m_queues.graphics.familyIndex = i;

                        VkBool32 presentSupport = VK_FALSE;
                        HR( vkGetPhysicalDeviceSurfaceSupportKHR( m_vkPhysicalDevice, i, m_vkSurface, &presentSupport ) );

                        if( m_queues.present.familyIndex == VK_QUEUE_FAMILY_IGNORED && presentSupport == VK_TRUE )
                        {
                            m_queues.present.familyIndex = i;
                        }
                    }

                    if( m_queues.transfer.familyIndex == VK_QUEUE_FAMILY_IGNORED &&
                        zp_flag32_is_set( queueFamilyProperty.queueFlags, VK_QUEUE_TRANSFER_BIT ) )
                    {
                        m_queues.transfer.familyIndex = i;
                    }

                    if( m_queues.compute.familyIndex == VK_QUEUE_FAMILY_IGNORED &&
                        zp_flag32_is_set( queueFamilyProperty.queueFlags, VK_QUEUE_COMPUTE_BIT ) )
                    {
                        m_queues.compute.familyIndex = i;
                    }
                }

                // find dedicated compute and transfer queues
                for( zp_size_t i = 0; i < queueFamilyCount; ++i )
                {
                    const VkQueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[ i ];
                    if( queueFamilyProperty.queueCount == 0 )
                    {
                        continue;
                    }

                    if( zp_flag32_is_set( queueFamilyProperty.queueFlags, VK_QUEUE_TRANSFER_BIT ) &&
                        !zp_flag32_any_set( queueFamilyProperty.queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) )
                    {
                        m_queues.transfer.familyIndex = i;
                    }

                    if( zp_flag32_is_set( queueFamilyProperty.queueFlags, VK_QUEUE_COMPUTE_BIT ) &&
                        !zp_flag32_is_set( queueFamilyProperty.queueFlags, VK_QUEUE_GRAPHICS_BIT ) )
                    {
                        m_queues.compute.familyIndex = i;
                    }
                }

                Set<zp_uint32_t, zp_hash64_t, CastEqualityComparer<zp_uint32_t, zp_hash64_t>> uniqueFamilyIndices( 4, MemoryLabels::Temp );
                uniqueFamilyIndices.add( m_queues.graphics.familyIndex );
                uniqueFamilyIndices.add( m_queues.transfer.familyIndex );
                uniqueFamilyIndices.add( m_queues.compute.familyIndex );
                uniqueFamilyIndices.add( m_queues.present.familyIndex );

                Vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos( 4, MemoryLabels::Temp );

                const zp_float32_t queuePriority = 1.0F;
                for( const zp_uint32_t& queueFamily : uniqueFamilyIndices )
                {
                    deviceQueueCreateInfos.pushBack( {
                        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                        .queueFamilyIndex = queueFamily,
                        .queueCount = 1,
                        .pQueuePriorities = &queuePriority,
                    } );
                }

                //
                uint32_t extensionCount = 0;
                HR( vkEnumerateDeviceExtensionProperties( m_vkPhysicalDevice, nullptr, &extensionCount, VK_NULL_HANDLE ) );

                Vector<VkExtensionProperties> availableDeviceExtensions( extensionCount, MemoryLabels::Temp );
                availableDeviceExtensions.resize_unsafe( extensionCount );

                HR( vkEnumerateDeviceExtensionProperties( m_vkPhysicalDevice, nullptr, &extensionCount, availableDeviceExtensions.data() ) );

                //
                Vector<const char*> supportedExtensions( ZP_ARRAY_SIZE( kDeviceExtensions ), MemoryLabels::Temp );

                VkPhysicalDeviceFeatures2 deviceFeatures {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                };

                VkPhysicalDeviceVulkan11Features deviceFeaturesVulkan11 {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
                };
                pNextPushFront( deviceFeatures, deviceFeaturesVulkan11 );

                VkPhysicalDeviceVulkan12Features deviceFeaturesVulkan12 {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                };
                pNextPushFront( deviceFeatures, deviceFeaturesVulkan12 );

                VkPhysicalDeviceVulkan13Features deviceFeaturesVulkan13 {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                };
                pNextPushFront( deviceFeatures, deviceFeaturesVulkan13 );

                VkPhysicalDeviceMaintenance5FeaturesKHR maintenance5Features {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR,
                };

                if( IsExtensionSupported( VK_KHR_MAINTENANCE_5_EXTENSION_NAME, availableDeviceExtensions ) )
                {
                    pNextPushFront( deviceFeatures, maintenance5Features );
                    supportedExtensions.pushBack( VK_KHR_MAINTENANCE_5_EXTENSION_NAME );
                }

                VkPhysicalDeviceMaintenance6FeaturesKHR maintenance6Features {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES_KHR,
                };

                if( IsExtensionSupported( VK_KHR_MAINTENANCE_6_EXTENSION_NAME, availableDeviceExtensions ) )
                {
                    pNextPushFront( deviceFeatures, maintenance6Features );
                    supportedExtensions.pushBack( VK_KHR_MAINTENANCE_6_EXTENSION_NAME );
                }

                VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
                };

                if( IsExtensionSupported( VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME, availableDeviceExtensions ) )
                {
                    pNextPushFront( deviceFeatures, extendedDynamicStateFeatures );
                    supportedExtensions.pushBack( VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME );
                }

                VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamicState2Features {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT,
                };

                if( IsExtensionSupported( VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME, availableDeviceExtensions ) )
                {
                    pNextPushFront( deviceFeatures, extendedDynamicState2Features );
                    supportedExtensions.pushBack( VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME );
                }

                VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,
                };

                if( IsExtensionSupported( VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME, availableDeviceExtensions ) )
                {
                    pNextPushFront( deviceFeatures, extendedDynamicState3Features );
                    supportedExtensions.pushBack( VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME );
                }

                if( IsExtensionSupported( VK_KHR_SWAPCHAIN_EXTENSION_NAME, availableDeviceExtensions ) )
                {
                    supportedExtensions.pushBack( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
                }

                vkGetPhysicalDeviceFeatures2( m_vkPhysicalDevice, &deviceFeatures );

                ZP_ASSERT( deviceFeaturesVulkan13.dynamicRendering );
                ZP_ASSERT( deviceFeaturesVulkan13.maintenance4 );
                ZP_ASSERT( maintenance5Features.maintenance5 );
                //ZP_ASSERT( maintenance6Features.maintenance6 );

                //
                VkPhysicalDeviceProperties2 deviceProperties {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                };

                VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProperties {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR,
                };
                pNextPushFront( deviceProperties, pushDescriptorProperties );

                vkGetPhysicalDeviceProperties2( m_vkPhysicalDevice, &deviceProperties );

                PrintPhysicalDeviceInfo( deviceProperties.properties );

                //
                VkDeviceCreateInfo localDeviceCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                    .queueCreateInfoCount = static_cast<uint32_t>( deviceQueueCreateInfos.size() ),
                    .pQueueCreateInfos = deviceQueueCreateInfos.data(),
                    .enabledLayerCount = ZP_ARRAY_SIZE( kValidationLayers ),
                    .ppEnabledLayerNames = kValidationLayers,
                    .enabledExtensionCount = static_cast<zp_uint32_t>( supportedExtensions.size() ),
                    .ppEnabledExtensionNames = supportedExtensions.data(),
                };
                pNext( localDeviceCreateInfo, deviceFeatures );

                HR( vkCreateDevice( m_vkPhysicalDevice, &localDeviceCreateInfo, &m_vkAllocationCallbacks, &m_vkLocalDevice ) );

                volkLoadDevice( m_vkLocalDevice );

                //
                vkGetDeviceQueue( m_vkLocalDevice, m_queues.graphics.familyIndex, m_queues.graphics.queueIndex, &m_queues.graphics.vkQueue );
                vkGetDeviceQueue( m_vkLocalDevice, m_queues.transfer.familyIndex, m_queues.transfer.queueIndex, &m_queues.transfer.vkQueue );
                vkGetDeviceQueue( m_vkLocalDevice, m_queues.compute.familyIndex, m_queues.compute.queueIndex, &m_queues.compute.vkQueue );
                vkGetDeviceQueue( m_vkLocalDevice, m_queues.present.familyIndex, m_queues.present.queueIndex, &m_queues.present.vkQueue );

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_QUEUE, m_queues.graphics.vkQueue, "Graphics Queue" );
                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_QUEUE, m_queues.transfer.vkQueue, "Transfer Queue" );
                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_QUEUE, m_queues.compute.vkQueue, "Compute Queue" );
                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_QUEUE, m_queues.present.vkQueue, "Present Queue" );
            }

            // create pipeline cache
            {
                // TODO: load cached pipeline cache
                const Memory loadedPipelineCache {};

                const VkPipelineCacheCreateInfo pipelineCacheCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
                    .initialDataSize = loadedPipelineCache.size,
                    .pInitialData = loadedPipelineCache.ptr,
                };

                HR( vkCreatePipelineCache( m_vkLocalDevice, &pipelineCacheCreateInfo, &m_vkAllocationCallbacks, &m_vkPipelineCache ) );

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_PIPELINE_CACHE, m_vkPipelineCache, "Pipeline Cache" );
            }

            // create descriptor pool
            {
                const zp_uint32_t kDefaultDescriptorCount = 32;

                // @formatter:off
                const VkDescriptorPoolSize poolSizes[] {
                    { .type = VK_DESCRIPTOR_TYPE_SAMPLER,                   .descriptorCount = kDefaultDescriptorCount },
                    { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,    .descriptorCount = kDefaultDescriptorCount },
                    { .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,             .descriptorCount = kDefaultDescriptorCount },
                    { .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,             .descriptorCount = kDefaultDescriptorCount },
                    { .type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,      .descriptorCount = kDefaultDescriptorCount },
                    { .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,      .descriptorCount = kDefaultDescriptorCount },
                    { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,            .descriptorCount = kDefaultDescriptorCount },
                    { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,            .descriptorCount = kDefaultDescriptorCount },
                    { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,    .descriptorCount = kDefaultDescriptorCount },
                    { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,    .descriptorCount = kDefaultDescriptorCount },
                    { .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,          .descriptorCount = kDefaultDescriptorCount }
                };
                // @formatter:on

                const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                    .maxSets = 128,
                    .poolSizeCount = ZP_ARRAY_SIZE( poolSizes ),
                    .pPoolSizes = poolSizes,
                };

                HR( vkCreateDescriptorPool( m_vkLocalDevice, &descriptorPoolCreateInfo, &m_vkAllocationCallbacks, &m_vkDescriptorPool ) );

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_DESCRIPTOR_POOL, m_vkDescriptorPool, "Descriptor Pool" );
            }

            // create transient command pool
            {
                const VkCommandPoolCreateInfo transientCommandPoolCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                    .queueFamilyIndex = m_queues.graphics.familyIndex,
                };

                HR( vkCreateCommandPool( m_vkLocalDevice, &transientCommandPoolCreateInfo, &m_vkAllocationCallbacks, &m_vkTransientCommandPool ) );

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_COMMAND_POOL, m_vkTransientCommandPool, "Transient Command Pool" );
            }
        }

        void VulkanContext::Destroy()
        {
            ZP_ASSERT( m_vkLocalDevice );

            HR( vkDeviceWaitIdle( m_vkLocalDevice ) );

            // TODO: write out pipeline cache?
            vkDestroyPipelineCache( m_vkLocalDevice, m_vkPipelineCache, &m_vkAllocationCallbacks );

            vkDestroyDescriptorPool( m_vkLocalDevice, m_vkDescriptorPool, &m_vkAllocationCallbacks );

#if ZP_DEBUG
            CallDebugUtil( vkDestroyDebugUtilsMessengerEXT, m_vkInstance, m_vkInstance, m_vkDebugMessenger, &m_vkAllocationCallbacks );
#endif // ZP_DEBUG

            vkDestroyDevice( m_vkLocalDevice, &m_vkAllocationCallbacks );
            vkDestroyInstance( m_vkInstance, &m_vkAllocationCallbacks );

            volkFinalize();

            m_vkInstance = nullptr;
            m_vkLocalDevice = nullptr;

#if ZP_DEBUG
            zp_zero_memory( &m_vkAllocationCallbacks );
#endif // ZP_DEBUG

            m_vkDescriptorPool = nullptr;
            m_vkPipelineCache = nullptr;
        }

        void VulkanContext::EndFrame( VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence fence )
        {
            VkSemaphoreSubmitInfo waitSemaphores {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = waitSemaphore,
                .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            };
            VkCommandBufferSubmitInfo commandBuffers {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = nullptr,
            };
            VkSemaphoreSubmitInfo signalSemaphores {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = signalSemaphore,
                .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            };

            const VkSubmitInfo2 submitInfo {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .waitSemaphoreInfoCount = 1,
                .pWaitSemaphoreInfos = &waitSemaphores,
                .commandBufferInfoCount = 0,
                .pCommandBufferInfos = &commandBuffers,
                .signalSemaphoreInfoCount = 1,
                .pSignalSemaphoreInfos = &signalSemaphores,
            };

            HR( vkQueueSubmit2( m_queues.graphics.vkQueue, 1, &submitInfo, fence ) );
        }

        void VulkanContext::QueueDestroy( void* handle, VkObjectType objectType )
        {
            const zp_size_t index = m_delayedDestroyed.pushBackEmptyRangeAtomic( 1, false );
            m_delayedDestroyed[ index ] = {
                .frameIndex = m_frameIndex,
                .vkInstance = m_vkInstance,
                .vkLocalDevice = m_vkLocalDevice,
                .vkAllocatorCallbacks = &m_vkAllocationCallbacks,
                .order = index,
                .vkHandle = handle,
                .vkObjectType = objectType,
            };
        }
    };

    //
    //
    //

    namespace
    {
        class VulkanSwapchain
        {
        public:

            void Initialize( VulkanContext* context, WindowHandle windowHandle );

            void Destroy();

            void AcquireNextImage();

            void Present();

            void Rebuild();

            VkSemaphore GetWaitSemaphore() const
            {
                return m_vkSwapchainAcquireSemaphores[ m_currentFrameIndex ];
            }

            VkSemaphore GetSignalSemaphore() const
            {
                return m_vkRenderFinishedSemaphores[ m_currentFrameIndex ];
            }

            VkFence GetFrameInFlightFence() const
            {
                return m_vkFrameInFlightFences[ m_currentFrameIndex ];
            }

        private:
            void CreateSwapchain();

            void DestroySwapchain();

            WindowHandle m_windowHandle;

            VulkanContext* m_context;

            VkSwapchainKHR m_vkSwapchain;
            VkSurfaceFormatKHR m_vkSwapChainFormat;
            VkColorSpaceKHR vkSwapchainColorSpace;

            VkExtent2D m_vkSwapchainExtent;
            VkRenderPass m_vkSwapchainDefaultRenderPass;

            FixedArray<VkImage, kBufferedFrameCount> m_vkSwapchainImages;
            FixedArray<VkImageView, kBufferedFrameCount> m_vkSwapchainImageViews;
            FixedArray<VkFramebuffer, kBufferedFrameCount> m_vkSwapchainFrameBuffers;

            FixedArray<VkSemaphore, kBufferedFrameCount> m_vkSwapchainAcquireSemaphores;
            FixedArray<VkSemaphore, kBufferedFrameCount> m_vkRenderFinishedSemaphores;
            FixedArray<VkFence, kBufferedFrameCount> m_vkFrameInFlightFences;
            FixedArray<VkFence, kBufferedFrameCount> m_vkSwapchainImageAcquiredFences;
            FixedArray<zp_uint32_t, kBufferedFrameCount> m_swapchainImageIndices;

            zp_size_t m_currentFrameIndex;
            zp_uint32_t m_maxFramesInFlight;

            zp_bool_t m_requiresRebuild;
            zp_bool_t m_vSync;
        };

        void VulkanSwapchain::Initialize( VulkanContext* context, WindowHandle windowHandle )
        {
            m_context = context;
            m_windowHandle = windowHandle;

            CreateSwapchain();
        }

        void VulkanSwapchain::Destroy()
        {
            DestroySwapchain();

            m_context = nullptr;
            m_windowHandle = {};
        }

        void VulkanSwapchain::AcquireNextImage()
        {
            ZP_PROFILE_CPU_BLOCK();

            if( m_requiresRebuild )
            {
                HR( vkDeviceWaitIdle( m_context->GetLocalDevice() ) );

                DestroySwapchain();
                CreateSwapchain();

                m_requiresRebuild = false;
            }

            HR( vkWaitForFences( m_context->GetLocalDevice(), 1, &m_vkFrameInFlightFences[ m_currentFrameIndex ], VK_TRUE, zp_limit<zp_uint64_t>::max() ) );

            const VkResult acquireNextImageResult = vkAcquireNextImageKHR( m_context->GetLocalDevice(), m_vkSwapchain, zp_limit<zp_uint64_t>::max(), m_vkSwapchainAcquireSemaphores[ m_currentFrameIndex ], VK_NULL_HANDLE, &m_swapchainImageIndices[ m_currentFrameIndex ] );

            if( acquireNextImageResult != VK_SUCCESS )
            {
                if( acquireNextImageResult == VK_ERROR_OUT_OF_DATE_KHR || acquireNextImageResult == VK_SUBOPTIMAL_KHR )
                {
                    m_requiresRebuild = true;
                }
                else
                {
                    HR( acquireNextImageResult );
                }
            }

            HR( vkResetFences( m_context->GetLocalDevice(), 1, &m_vkFrameInFlightFences[ m_currentFrameIndex ] ) );
        }

        void VulkanSwapchain::Present()
        {
            ZP_PROFILE_CPU_BLOCK();

            const VkPresentInfoKHR presentInfo {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &m_vkRenderFinishedSemaphores[ m_currentFrameIndex ],
                .swapchainCount = 1,
                .pSwapchains = &m_vkSwapchain,
                .pImageIndices = &m_swapchainImageIndices[ m_currentFrameIndex ],
            };

            HR( vkQueuePresentKHR( m_context->GetQueues().present.vkQueue, &presentInfo ) );

            m_currentFrameIndex = ( m_currentFrameIndex + 1 ) % m_maxFramesInFlight;
        }

        void VulkanSwapchain::Rebuild()
        {
            m_requiresRebuild = true;
        }

        void VulkanSwapchain::CreateSwapchain()
        {
            //
            VkSurfaceCapabilitiesKHR surfaceCapabilities {};
            HR( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_context->GetPhysicalDevice(), m_context->GetSurface(), &surfaceCapabilities ) );

            m_vkSwapchainExtent = ChooseSwapChainExtent( surfaceCapabilities, surfaceCapabilities.currentExtent );
            if( m_vkSwapchainExtent.width == 0 && m_vkSwapchainExtent.height == 0 )
            {
                return;
            }

            //
            uint32_t formatCount = 0;
            HR( vkGetPhysicalDeviceSurfaceFormatsKHR( m_context->GetPhysicalDevice(), m_context->GetSurface(), &formatCount, VK_NULL_HANDLE ) );

            Vector<VkSurfaceFormatKHR> supportedSurfaceFormats( formatCount, MemoryLabels::Temp );
            supportedSurfaceFormats.resize_unsafe( formatCount );

            HR( vkGetPhysicalDeviceSurfaceFormatsKHR( m_context->GetPhysicalDevice(), m_context->GetSurface(), &formatCount, supportedSurfaceFormats.data() ) );

            //
            uint32_t presentModeCount = 0;
            HR( vkGetPhysicalDeviceSurfacePresentModesKHR( m_context->GetPhysicalDevice(), m_context->GetSurface(), &presentModeCount, VK_NULL_HANDLE ) );

            Vector<VkPresentModeKHR> presentModes( presentModeCount, MemoryLabels::Temp );
            presentModes.resize_unsafe( presentModeCount );

            HR( vkGetPhysicalDeviceSurfacePresentModesKHR( m_context->GetPhysicalDevice(), m_context->GetSurface(), &presentModeCount, presentModes.data() ) );

            const VkPresentModeKHR presentMode = ChooseSwapChainPresentMode( presentModes, m_vSync );

            // TODO: make configurable?
            const FixedArray<VkSurfaceFormatKHR, 2> preferredSurfaceFormats = {
                VkSurfaceFormatKHR { .format = VK_FORMAT_B8G8R8A8_SNORM, .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR },
                VkSurfaceFormatKHR { .format = VK_FORMAT_R8G8B8A8_SNORM, .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR },
            };

            m_vkSwapChainFormat = ChooseSwapChainSurfaceFormat( supportedSurfaceFormats, preferredSurfaceFormats.asReadonly() );

            //
            // TODO: make this configurable?
            const zp_uint32_t preferredImageCount = 3;
            uint32_t imageCount = zp_max( preferredImageCount, surfaceCapabilities.minImageCount );
            if( surfaceCapabilities.maxImageCount > 0 )
            {
                imageCount = zp_min( imageCount, surfaceCapabilities.maxImageCount );
            }

            m_maxFramesInFlight = imageCount;
            ZP_ASSERT_MSG_ARGS( m_maxFramesInFlight < kBufferedFrameCount, "Increase buffered frames %d < %d", m_maxFramesInFlight, kBufferedFrameCount );

            //
            const Queues& queues = m_context->GetQueues();
            const bool useConcurrentSharingMode = queues.graphics.familyIndex != queues.present.familyIndex;

            const FixedArray<zp_uint32_t, 2> indices = { queues.graphics.familyIndex, queues.present.familyIndex };

            const VkSwapchainCreateInfoKHR swapChainCreateInfo {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface = m_context->GetSurface(),
                .minImageCount = m_maxFramesInFlight,
                .imageFormat = m_vkSwapChainFormat.format,
                .imageColorSpace = m_vkSwapChainFormat.colorSpace,
                .imageExtent = m_vkSwapchainExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                .imageSharingMode = useConcurrentSharingMode ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = useConcurrentSharingMode ? static_cast<zp_uint32_t>( indices.length() ) : 0,
                .pQueueFamilyIndices = useConcurrentSharingMode ? indices.data() : nullptr,
                .preTransform = surfaceCapabilities.currentTransform,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = presentMode,
                .clipped = VK_TRUE,
                .oldSwapchain = m_vkSwapchain
            };

            HR( vkCreateSwapchainKHR( m_context->GetLocalDevice(), &swapChainCreateInfo, m_context->GetAllocationCallbacks(), &m_vkSwapchain ) );

            SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), VK_OBJECT_TYPE_SWAPCHAIN_KHR, m_vkSwapchain, "Swapchain" );

            if( swapChainCreateInfo.oldSwapchain != VK_NULL_HANDLE )
            {
                // TODO: delayed destroy?
                m_context->QueueDestroy( swapChainCreateInfo.oldSwapchain );
                //vkDestroySwapchainKHR( m_context->GetLocalDevice(), swapChainCreateInfo.oldSwapchain, m_context->GetAllocationCallbacks() );
            }

            //
            zp_uint32_t swapchainImageCount = 0;
            HR( vkGetSwapchainImagesKHR( m_context->GetLocalDevice(), m_vkSwapchain, &swapchainImageCount, VK_NULL_HANDLE ) );
            ZP_ASSERT_MSG( m_maxFramesInFlight == swapchainImageCount, "Mismatch swapchain images and frames in flight" );

            HR( vkGetSwapchainImagesKHR( m_context->GetLocalDevice(), m_vkSwapchain, &swapchainImageCount, m_vkSwapchainImages.data() ) );

#if 0
            {
                if( m_swapchainData.vkSwapchainDefaultRenderPass )
                {
                    const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 1, false );

                    m_delayedDestroy[ index ] = {
                        .frameIndex = m_currentFrameIndex,
                        .handle = m_swapchainData.vkSwapchainDefaultRenderPass,
                        .allocator = &m_vkAllocationCallbacks,
                        .localDevice = m_vkLocalDevice,
                        .instance = m_vkInstance,
                        .order = index,
                        .type = DelayedDestroyType::RenderPass,
                    };

                    m_swapchainData.vkSwapchainDefaultRenderPass = VK_NULL_HANDLE;
                }

                VkAttachmentDescription colorAttachments[] {
                    {
                        .format = m_vkSwapChainFormat,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    }
                };

                VkAttachmentReference colorAttachmentRefs[] {
                    {
                        .attachment = 0,
                        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    }
                };

                VkSubpassDescription subPassDescriptions[] {
                    {
                        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                        .colorAttachmentCount = ZP_ARRAY_SIZE( colorAttachmentRefs ),
                        .pColorAttachments = colorAttachmentRefs,
                    }
                };

                VkSubpassDependency subPassDependencies[] {
                    {
                        .srcSubpass = VK_SUBPASS_EXTERNAL,
                        .dstSubpass = 0,
                        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .srcAccessMask = 0,
                        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    }
                };

                VkRenderPassCreateInfo renderPassInfo {
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                    .attachmentCount = ZP_ARRAY_SIZE( colorAttachments ),
                    .pAttachments = colorAttachments,
                    .subpassCount = ZP_ARRAY_SIZE( subPassDescriptions ),
                    .pSubpasses = subPassDescriptions,
                    .dependencyCount = ZP_ARRAY_SIZE( subPassDependencies ),
                    .pDependencies = subPassDependencies,
                };

                HR( vkCreateRenderPass( m_vkLocalDevice, &renderPassInfo, &m_vkAllocationCallbacks, &m_swapchainData.vkSwapchainDefaultRenderPass ) );

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_RENDER_PASS, m_swapchainData.vkSwapchainDefaultRenderPass, "Swapchain Default Render Pass" );
            }
#endif
            for( zp_size_t i = 0; i < m_maxFramesInFlight; ++i )
            {
                const VkImageViewCreateInfo imageViewCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = m_vkSwapchainImages[ i ],
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = m_vkSwapChainFormat.format,
                    .components {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                    .subresourceRange {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    }
                };

                if( m_vkSwapchainImageViews[ i ] != VK_NULL_HANDLE )
                {
                    m_context->QueueDestroy( m_vkSwapchainImageViews[ i ] );
                    m_vkSwapchainImageViews[ i ] = VK_NULL_HANDLE;
                };

                HR( vkCreateImageView( m_context->GetLocalDevice(), &imageViewCreateInfo, m_context->GetAllocationCallbacks(), &m_vkSwapchainImageViews[ i ] ) );

                SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, m_vkSwapchainImageViews[ i ], "Swapchain Image View %d", i );

                //
                VkImageView attachments[] { m_vkSwapchainImageViews[ i ] };

                const VkFramebufferCreateInfo framebufferCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .renderPass = nullptr, //m_swapchainData.vkSwapchainDefaultRenderPass,
                    .attachmentCount = 0, //ZP_ARRAY_SIZE( attachments ),
                    .pAttachments = attachments,
                    .width = m_vkSwapchainExtent.width,
                    .height = m_vkSwapchainExtent.height,
                    .layers = 1,
                };

                if( m_vkSwapchainFrameBuffers[ i ] != VK_NULL_HANDLE )
                {
                    m_context->QueueDestroy( m_vkSwapchainFrameBuffers[ i ] );
                    m_vkSwapchainFrameBuffers[ i ] = VK_NULL_HANDLE;
                }

                //HR( vkCreateFramebuffer( m_context->GetLocalDevice(), &framebufferCreateInfo, m_context->GetAllocationCallbacks(), &m_vkSwapchainFrameBuffers[ i ] ) );

                //SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), VK_OBJECT_TYPE_FRAMEBUFFER, m_vkSwapchainFrameBuffers[ i ], "Swapchain Framebuffer %d", i );
            }

            //
            const VkSemaphoreCreateInfo semaphoreCreateInfo {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            };

            const VkFenceCreateInfo fenceCreateInfo {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };

            for( zp_size_t i = 0; i < m_maxFramesInFlight; ++i )
            {
                HR( vkCreateSemaphore( m_context->GetLocalDevice(), &semaphoreCreateInfo, m_context->GetAllocationCallbacks(), &m_vkSwapchainAcquireSemaphores[ i ] ) );
                HR( vkCreateSemaphore( m_context->GetLocalDevice(), &semaphoreCreateInfo, m_context->GetAllocationCallbacks(), &m_vkRenderFinishedSemaphores[ i ] ) );
                HR( vkCreateFence( m_context->GetLocalDevice(), &fenceCreateInfo, m_context->GetAllocationCallbacks(), &m_vkFrameInFlightFences[ i ] ) );
                HR( vkCreateFence( m_context->GetLocalDevice(), &fenceCreateInfo, m_context->GetAllocationCallbacks(), &m_vkSwapchainImageAcquiredFences[ i ] ) );

                SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), VK_OBJECT_TYPE_SEMAPHORE, m_vkSwapchainAcquireSemaphores[ i ], "Swapchain Acquire Semaphore %d", i );
                SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), VK_OBJECT_TYPE_SEMAPHORE, m_vkRenderFinishedSemaphores[ i ], "Render Finished Semaphore %d", i );
                SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), VK_OBJECT_TYPE_FENCE, m_vkFrameInFlightFences[ i ], "In Flight Fence %d", i );
                SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), VK_OBJECT_TYPE_FENCE, m_vkSwapchainImageAcquiredFences[ i ], "Swapchain Image Acquire Fence %d", i );
            }

            // transition swapchain images to present layout
            VkCommandBuffer cmdBuffer = RequestSingleUseCommandBuffer( m_context->GetLocalDevice(), m_context->GetTransientCommandPool() );
            for( zp_size_t i = 0; i < m_maxFramesInFlight; ++i )
            {
                CmdTransitionImageLayout( cmdBuffer, m_vkSwapchainImages[ i ], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );
            }
            ReleaseSingleUseCommandBuffer( m_context->GetLocalDevice(), m_context->GetTransientCommandPool(), cmdBuffer, m_context->GetQueues().graphics.vkQueue, m_context->GetAllocationCallbacks() );
        }

        void VulkanSwapchain::DestroySwapchain()
        {
            for( zp_size_t i = 0; i < m_maxFramesInFlight; ++i )
            {
                vkDestroySemaphore( m_context->GetLocalDevice(), m_vkSwapchainAcquireSemaphores[ i ], m_context->GetAllocationCallbacks() );
                vkDestroySemaphore( m_context->GetLocalDevice(), m_vkRenderFinishedSemaphores[ i ], m_context->GetAllocationCallbacks() );
                vkDestroyFence( m_context->GetLocalDevice(), m_vkFrameInFlightFences[ i ], m_context->GetAllocationCallbacks() );
                vkDestroyFence( m_context->GetLocalDevice(), m_vkSwapchainImageAcquiredFences[ i ], m_context->GetAllocationCallbacks() );
            }

            vkDestroyRenderPass( m_context->GetLocalDevice(), m_vkSwapchainDefaultRenderPass, m_context->GetAllocationCallbacks() );
            m_vkSwapchainDefaultRenderPass = VK_NULL_HANDLE;

            for( zp_size_t i = 0; i < m_maxFramesInFlight; ++i )
            {
                VkFramebuffer swapChainFramebuffer = m_vkSwapchainFrameBuffers[ i ];
                vkDestroyFramebuffer( m_context->GetLocalDevice(), swapChainFramebuffer, m_context->GetAllocationCallbacks() );
            }

            for( zp_size_t i = 0; i < m_maxFramesInFlight; ++i )
            {
                VkImageView swapChainImageView = m_vkSwapchainImageViews[ i ];
                vkDestroyImageView( m_context->GetLocalDevice(), swapChainImageView, m_context->GetAllocationCallbacks() );
            }

            vkDestroySwapchainKHR( m_context->GetLocalDevice(), m_vkSwapchain, m_context->GetAllocationCallbacks() );
            m_vkSwapchain = VK_NULL_HANDLE;
        }
    }

    //
    //
    //

    namespace
    {
        class VulkanGraphicsDevice : public GraphicsDevice
        {
        public:
            explicit VulkanGraphicsDevice( MemoryLabel memoryLabel );

            void Initialize( const GraphicsDeviceDesc& graphicsDeviceDesc );

            void Destroy();

            void BeginFrame();

            void EndFrame();

        private:
            VulkanContext m_context;
            VulkanSwapchain m_swapchain;

        public:
            MemoryLabel memoryLabel;
        };

        VulkanGraphicsDevice::VulkanGraphicsDevice( MemoryLabel memoryLabel )
            : memoryLabel( memoryLabel )
        {
        }

        void VulkanGraphicsDevice::Initialize( const zp::GraphicsDeviceDesc& graphicsDeviceDesc )
        {
            m_context.Initialize( graphicsDeviceDesc );

            m_swapchain.Initialize( &m_context, graphicsDeviceDesc.windowHandle );
        }

        void VulkanGraphicsDevice::Destroy()
        {
            m_swapchain.Destroy();

            m_context.Destroy();
        }

        void VulkanGraphicsDevice::BeginFrame()
        {
            m_swapchain.AcquireNextImage();


        }

        void VulkanGraphicsDevice::EndFrame()
        {
            m_context.EndFrame( m_swapchain.GetWaitSemaphore(), m_swapchain.GetSignalSemaphore(), m_swapchain.GetFrameInFlightFence() );

            m_swapchain.Present();
        }
    };

    //
    //
    //

    namespace internal
    {
        GraphicsDevice* CreateVulkanGraphicsDevice( MemoryLabel memoryLabel )
        {
            return ZP_NEW( memoryLabel, VulkanGraphicsDevice );
        }

        void DestroyVulkanGraphicsDevice( GraphicsDevice* graphicsDevice )
        {
            ZP_SAFE_DELETE( VulkanGraphicsDevice, graphicsDevice );
        }
    }

    //
    // Old
    //
#if 0
    VulkanGraphicsDevice::VulkanGraphicsDevice( MemoryLabel memoryLabel, const GraphicsDeviceDesc& graphicsDeviceDesc )
        : GraphicsDevice()
        , m_perFrameData {}
        , m_swapchainData {}
        , m_vkInstance( VK_NULL_HANDLE )
        , m_vkSurface( VK_NULL_HANDLE )
        , m_vkPhysicalDevice( VK_NULL_HANDLE )
        , m_vkLocalDevice( VK_NULL_HANDLE )
        , m_vkAllocationCallbacks {
            .pUserData = GetAllocator( memoryLabel ),
            .pfnAllocation = AllocationCallback,
            .pfnReallocation = ReallocationCallback,
            .pfnFree = FreeCallback,
        }
        , m_vkRenderQueues {}
        , m_vkPipelineCache( VK_NULL_HANDLE )
        , m_vkDescriptorPool( VK_NULL_HANDLE )
        , m_vkCommandPools( nullptr )
        , m_commandPoolCount( 0 )
#if ZP_DEBUG
        , m_vkDebugMessenger( VK_NULL_HANDLE )
#endif
        , m_descriptorSetLayoutCache( memoryLabel, 64, memoryLabel )
        , m_samplerCache( memoryLabel, 16, memoryLabel )
        , m_delayedDestroy( 64, memoryLabel )
        , m_stagingBuffer {}
        , m_queueFamilies {
            .graphicsFamily = VK_QUEUE_FAMILY_IGNORED,
            .transferFamily = VK_QUEUE_FAMILY_IGNORED,
            .computeFamily = VK_QUEUE_FAMILY_IGNORED,
            .presentFamily = VK_QUEUE_FAMILY_IGNORED
        }
        //, m_commandQueueCount( 0 )
        //, m_commandQueues( 4, memoryLabel )
        , m_currentFrameIndex( 0 )
        , memoryLabel( memoryLabel )
    {
        zp_zero_memory_array( m_perFrameData );

        const VkApplicationInfo applicationInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = !graphicsDeviceDesc.appName.empty() ? graphicsDeviceDesc.appName.c_str() : "ZeroPoint Application",
            .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
            .pEngineName = "ZeroPoint",
            .engineVersion = VK_MAKE_VERSION( ZP_VERSION_MAJOR, ZP_VERSION_MINOR, ZP_VERSION_PATCH ),
            .apiVersion = VK_API_VERSION_1_1,
        };

        const char* kExtensionNames[] {
            VK_KHR_SURFACE_EXTENSION_NAME,
#if ZP_OS_WINDOWS
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if ZP_DEBUG
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        };

        const char* kValidationLayers[] {
#if ZP_DEBUG
            "VK_LAYER_KHRONOS_validation"
#endif
        };

        // create instance
        VkInstanceCreateInfo instanceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = ZP_ARRAY_SIZE( kValidationLayers ),
            .ppEnabledLayerNames = kValidationLayers,
            .enabledExtensionCount = ZP_ARRAY_SIZE( kExtensionNames ),
            .ppEnabledExtensionNames = kExtensionNames,
        };

#if ZP_DEBUG
        // add debug info to create instance
        VkDebugUtilsMessengerCreateInfoEXT createInstanceDebugMessengerInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = DebugCallback,
            .pUserData = nullptr, // Optional
        };

        //instanceCreateInfo.pNext = &createInstanceDebugMessengerInfo;
        pNextPushFront( instanceCreateInfo, createInstanceDebugMessengerInfo );
#endif
        HR( vkCreateInstance( &instanceCreateInfo, &m_vkAllocationCallbacks, &m_vkInstance ) );

#if ZP_DEBUG
        // create debug messenger
        const VkDebugUtilsMessengerCreateInfoEXT createDebugMessengerInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = DebugCallback,
            .pUserData = nullptr, // Optional
        };

        HR( CallDebugUtilResult( vkCreateDebugUtilsMessengerEXT, m_vkInstance, m_vkInstance, &createDebugMessengerInfo, &m_vkAllocationCallbacks, &m_vkDebugMessenger ) );
        //HR( CreateDebugUtilsMessengerEXT( m_vkInstance, &createDebugMessengerInfo, nullptr, &m_vkDebugMessenger ));
#endif

        // select physical device
        {
            uint32_t physicalDeviceCount = 0;
            HR( vkEnumeratePhysicalDevices( m_vkInstance, &physicalDeviceCount, VK_NULL_HANDLE ) );

            Vector<VkPhysicalDevice> physicalDevices( physicalDeviceCount, MemoryLabels::Temp );
            physicalDevices.resize_unsafe( physicalDeviceCount );

            HR( vkEnumeratePhysicalDevices( m_vkInstance, &physicalDeviceCount, physicalDevices.data() ) );

            for( const VkPhysicalDevice& physicalDevice : physicalDevices )
            {
                if( IsPhysicalDeviceSuitable( physicalDevice, m_vkSurface, graphicsDeviceDesc ) )
                {
                    m_vkPhysicalDevice = physicalDevice;
                    break;
                }
            }

            ZP_ASSERT( m_vkPhysicalDevice );
            vkGetPhysicalDeviceMemoryProperties( m_vkPhysicalDevice, &m_vkPhysicalDeviceMemoryProperties );

            PrintPhysicalDeviceInfo( m_vkPhysicalDevice );
        }

        // create surface
        {
#if ZP_OS_WINDOWS
            auto hWnd = static_cast<HWND>( graphicsDeviceDesc.windowHandle );
            auto hInstance = reinterpret_cast<HINSTANCE>(::GetWindowLongPtr( hWnd, GWLP_HINSTANCE ));

            // create surface
            const VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo {
                .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                .hinstance = hInstance,
                .hwnd = hWnd,
            };

            HR( vkCreateWin32SurfaceKHR( m_vkInstance, &win32SurfaceCreateInfo, &m_vkAllocationCallbacks, &m_vkSurface ) );
#else
#error "Platform not defined to create VkSurface"
#endif
        }

        // create local device and queue families
        {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties( m_vkPhysicalDevice, &queueFamilyCount, VK_NULL_HANDLE );

            Vector<VkQueueFamilyProperties> queueFamilyProperties( queueFamilyCount, MemoryLabels::Temp );
            queueFamilyProperties.resize_unsafe( queueFamilyCount );

            vkGetPhysicalDeviceQueueFamilyProperties( m_vkPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data() );

            // find base queries
            for( zp_size_t i = 0; i < queueFamilyCount; ++i )
            {
                const VkQueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[ i ];

                if( m_queueFamilies.graphicsQueue == VK_QUEUE_FAMILY_IGNORED &&
                    queueFamilyProperty.queueCount > 0 &&
                    queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT )
                {
                    m_queueFamilies.graphicsQueue = i;

                    VkBool32 presentSupport = VK_FALSE;
                    HR( vkGetPhysicalDeviceSurfaceSupportKHR( m_vkPhysicalDevice, i, m_vkSurface, &presentSupport ) );

                    if( m_queueFamilies.presentQueue == VK_QUEUE_FAMILY_IGNORED && presentSupport )
                    {
                        m_queueFamilies.presentQueue = i;
                    }
                }

                if( m_queueFamilies.transferQueue == VK_QUEUE_FAMILY_IGNORED &&
                    queueFamilyProperty.queueCount > 0 &&
                    queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT )
                {
                    m_queueFamilies.transferQueue = i;
                }

                if( m_queueFamilies.computeQueue == VK_QUEUE_FAMILY_IGNORED &&
                    queueFamilyProperty.queueCount > 0 &&
                    queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT )
                {
                    m_queueFamilies.computeQueue = i;
                }
            }

            // find dedicated compute and transfer queues
            for( zp_size_t i = 0; i < queueFamilyCount; ++i )
            {
                const VkQueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[ i ];
                if( queueFamilyProperty.queueCount > 0 &&
                    queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT &&
                    !( queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT ) &&
                    !( queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT ) )
                {
                    m_queueFamilies.transferQueue = i;
                }

                if( queueFamilyProperty.queueCount > 0 &&
                    queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT &&
                    !( queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT ) )
                {
                    m_queueFamilies.computeQueue = i;
                }
            }

            Set<zp_uint32_t, zp_hash64_t, CastEqualityComparer<zp_uint32_t, zp_hash64_t>> uniqueFamilyIndices( 4, MemoryLabels::Temp );
            uniqueFamilyIndices.add( m_queueFamilies.graphicsQueue );
            uniqueFamilyIndices.add( m_queueFamilies.transferQueue );
            uniqueFamilyIndices.add( m_queueFamilies.computeQueue );
            uniqueFamilyIndices.add( m_queueFamilies.presentQueue );

            Vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos( 4, MemoryLabels::Temp );

            const zp_float32_t queuePriority = 1.0f;
            for( const zp_uint32_t& queueFamily : uniqueFamilyIndices )
            {
                VkDeviceQueueCreateInfo queueCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex = queueFamily,
                    .queueCount = 1,
                    .pQueuePriorities = &queuePriority,
                };

                deviceQueueCreateInfos.pushBack( queueCreateInfo );
            }

            const char* kDeviceExtensions[] {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };

            VkPhysicalDeviceFeatures deviceFeatures {};
            vkGetPhysicalDeviceFeatures( m_vkPhysicalDevice, &deviceFeatures );

            const VkPhysicalDeviceSynchronization2Features synchronization2Features {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
                .pNext = nullptr,
                .synchronization2 = VK_TRUE
            };

            const VkDeviceCreateInfo localDeviceCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &synchronization2Features,
                .queueCreateInfoCount = static_cast<uint32_t>( deviceQueueCreateInfos.size() ),
                .pQueueCreateInfos = deviceQueueCreateInfos.data(),
                .enabledLayerCount = ZP_ARRAY_SIZE( kValidationLayers ),
                .ppEnabledLayerNames = kValidationLayers,
                .enabledExtensionCount = ZP_ARRAY_SIZE( kDeviceExtensions ),
                .ppEnabledExtensionNames = kDeviceExtensions,
                .pEnabledFeatures = &deviceFeatures,
            };

            HR( vkCreateDevice( m_vkPhysicalDevice, &localDeviceCreateInfo, &m_vkAllocationCallbacks, &m_vkLocalDevice ) );

            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.graphicsQueue, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_GRAPHICS ] );
            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.transferQueue, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_TRANSFER ] );
            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.computeQueue, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_COMPUTE ] );
            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.presentQueue, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_PRESENT ] );

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_QUEUE, m_vkRenderQueues[ ZP_RENDER_QUEUE_GRAPHICS ], "Graphics Queue" );
            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_QUEUE, m_vkRenderQueues[ ZP_RENDER_QUEUE_TRANSFER ], "Transfer Queue" );
            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_QUEUE, m_vkRenderQueues[ ZP_RENDER_QUEUE_COMPUTE ], "Compute Queue" );
            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_QUEUE, m_vkRenderQueues[ ZP_RENDER_QUEUE_PRESENT ], "Present Queue" );
        }

        // create pipeline cache
        {
            const VkPipelineCacheCreateInfo pipelineCacheCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
                .initialDataSize = 0,
                .pInitialData = nullptr,
            };

            HR( vkCreatePipelineCache( m_vkLocalDevice, &pipelineCacheCreateInfo, &m_vkAllocationCallbacks, &m_vkPipelineCache ) );

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_PIPELINE_CACHE, m_vkPipelineCache, "Pipeline Cache" );
        }

        // create command pools
        {
            m_commandPoolCount = 0;
            m_vkCommandPools = ZP_MALLOC_T_ARRAY( memoryLabel, VkCommandPool, 3 * graphicsDeviceDesc.threadCount );

            //    VkCommandPoolCreateInfo commandPoolCreateInfo {
            //        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            //        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            //    };
            //
            //    commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.graphicsQueue;
            //    HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, nullptr, &m_vkGraphicsCommandPool ) );
            //
            //    commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.transferQueue;
            //    HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, nullptr, &m_vkTransferCommandPool ) );
            //
            //    commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.computeQueue;
            //    HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, nullptr, &m_vkComputeCommandPool ) );
        }

        // create staging buffer
        {
            GraphicsBufferDesc stagingBufferDesc {
                .name = "Staging Buffer",
                .size = graphicsDeviceDesc.stagingBufferSize,
                .usageFlags = ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_SRC | ZP_GRAPHICS_BUFFER_USAGE_STORAGE,
                .memoryPropertyFlags = ZP_MEMORY_PROPERTY_HOST_VISIBLE,
            };

            createBuffer( stagingBufferDesc, &m_stagingBuffer );
        }

        // create descriptor pool
        {
            const VkDescriptorPoolSize poolSizes[] {
                { .type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 128 },
                { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 128 },
                { .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 128 },
                { .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 128 },
                { .type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, .descriptorCount = 128 },
                { .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, .descriptorCount = 128 },
                { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 128 },
                { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 128 },
                { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = 128 },
                { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, .descriptorCount = 128 },
                { .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = 128 }
            };

            const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                .maxSets = 128,
                .poolSizeCount = ZP_ARRAY_SIZE( poolSizes ),
                .pPoolSizes = poolSizes,
            };

            HR( vkCreateDescriptorPool( m_vkLocalDevice, &descriptorPoolCreateInfo, &m_vkAllocationCallbacks, &m_vkDescriptorPool ) );

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_DESCRIPTOR_POOL, m_vkDescriptorPool, "Descriptor Pool" );
        }

        createPerFrameData();
    }

    VulkanGraphicsDevice::~VulkanGraphicsDevice()
    {
        HR( vkDeviceWaitIdle( m_vkLocalDevice ) );

        destroySwapchain();

        destroyPerFrameData();

        destroyBuffer( &m_stagingBuffer );

        destroyAllDelayedDestroy();

        auto b = m_descriptorSetLayoutCache.begin();
        auto e = m_descriptorSetLayoutCache.end();
        for( ; b != e; ++b )
        {
            vkDestroyDescriptorSetLayout( m_vkLocalDevice, b.value(), &m_vkAllocationCallbacks );
        }
        m_descriptorSetLayoutCache.clear();

        vkDestroyDescriptorPool( m_vkLocalDevice, m_vkDescriptorPool, &m_vkAllocationCallbacks );
        m_vkDescriptorPool = {};

        for( zp_size_t i = 0; i < m_commandPoolCount; ++i )
        {
            vkDestroyCommandPool( m_vkLocalDevice, m_vkCommandPools[ i ], &m_vkAllocationCallbacks );
        }
        ZP_FREE( memoryLabel, m_vkCommandPools );

        //vkDestroyCommandPool( m_vkLocalDevice, m_vkGraphicsCommandPool, nullptr );
        //vkDestroyCommandPool( m_vkLocalDevice, m_vkTransferCommandPool, nullptr );
        //vkDestroyCommandPool( m_vkLocalDevice, m_vkComputeCommandPool, nullptr );
        //m_vkGraphicsCommandPool = {};
        //m_vkTransferCommandPool = {};
        //m_vkComputeCommandPool = {};

        vkDestroyPipelineCache( m_vkLocalDevice, m_vkPipelineCache, &m_vkAllocationCallbacks );
        m_vkPipelineCache = {};

        vkDestroyDevice( m_vkLocalDevice, &m_vkAllocationCallbacks );
        m_vkLocalDevice = {};
        m_vkPhysicalDevice = {};
        zp_zero_memory_array( m_vkRenderQueues );

        vkDestroySurfaceKHR( m_vkInstance, m_vkSurface, &m_vkAllocationCallbacks );
        m_vkSurface = {};

#if ZP_DEBUG
        CallDebugUtil( vkDestroyDebugUtilsMessengerEXT, m_vkInstance, m_vkInstance, m_vkDebugMessenger, &m_vkAllocationCallbacks );
        m_vkDebugMessenger = {};
#endif

        vkDestroyInstance( m_vkInstance, &m_vkAllocationCallbacks );
        m_vkInstance = {};
    }

    void VulkanGraphicsDevice::createSwapchain( zp_handle_t windowHandle, zp_uint32_t width, zp_uint32_t height, int displayFormat, ColorSpace colorSpace )
    {
        m_swapchainData.windowHandle = windowHandle;

        VkSurfaceCapabilitiesKHR swapChainSupportDetails;
        HR( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_vkPhysicalDevice, m_vkSurface, &swapChainSupportDetails ) );

        m_swapchainData.m_vkSwapchainExtent = ChooseSwapChainExtent( swapChainSupportDetails, width, height );
        if( m_swapchainData.m_vkSwapchainExtent.width == 0 && m_swapchainData.m_vkSwapchainExtent.height == 0 )
        {
            return;
        }

        uint32_t formatCount = 0;
        HR( vkGetPhysicalDeviceSurfaceFormatsKHR( m_vkPhysicalDevice, m_vkSurface, &formatCount, VK_NULL_HANDLE ) );

        Vector<VkSurfaceFormatKHR> supportedSurfaceFormats( formatCount, MemoryLabels::Temp );
        supportedSurfaceFormats.resize_unsafe( formatCount );

        HR( vkGetPhysicalDeviceSurfaceFormatsKHR( m_vkPhysicalDevice, m_vkSurface, &formatCount, supportedSurfaceFormats.data() ) );

        uint32_t presentModeCount = 0;
        HR( vkGetPhysicalDeviceSurfacePresentModesKHR( m_vkPhysicalDevice, m_vkSurface, &presentModeCount, VK_NULL_HANDLE ) );

        Vector<VkPresentModeKHR> presentModes( presentModeCount, MemoryLabels::Temp );
        presentModes.resize_unsafe( presentModeCount );

        HR( vkGetPhysicalDeviceSurfacePresentModesKHR( m_vkPhysicalDevice, m_vkSurface, &presentModeCount, presentModes.data() ) );

        // TODO: remove when enums are created
        displayFormat = VK_FORMAT_B8G8R8A8_SNORM;

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapChainSurfaceFormat( supportedSurfaceFormats, displayFormat, colorSpace );
        m_swapchainData.vkSwapChainFormat = surfaceFormat.format;
        m_swapchainData.vkSwapchainColorSpace = Convert( colorSpace );

        VkPresentModeKHR presentMode = ChooseSwapChainPresentMode( presentModes, false );

        uint32_t imageCount = swapChainSupportDetails.minImageCount + 1;
        if( swapChainSupportDetails.maxImageCount > 0 )
        {
            imageCount = zp_min( imageCount, swapChainSupportDetails.maxImageCount );
        }

        VkSwapchainCreateInfoKHR swapChainCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_vkSurface,
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = m_swapchainData.m_vkSwapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .preTransform = swapChainSupportDetails.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = m_swapchainData.vkSwapchain
        };

        const uint32_t queueFamilyIndices[] = { m_queueFamilies.graphicsQueue, m_queueFamilies.presentQueue };

        if( m_queueFamilies.graphicsQueue != m_queueFamilies.presentQueue )
        {
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapChainCreateInfo.queueFamilyIndexCount = ZP_ARRAY_SIZE( queueFamilyIndices );
            swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapChainCreateInfo.queueFamilyIndexCount = 0; // Optional
            swapChainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        HR( vkCreateSwapchainKHR( m_vkLocalDevice, &swapChainCreateInfo, &m_vkAllocationCallbacks, &m_swapchainData.vkSwapchain ) );

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_SWAPCHAIN_KHR, m_swapchainData.vkSwapchain, "Swapchain" );

        if( swapChainCreateInfo.oldSwapchain != VK_NULL_HANDLE )
        {
            vkDestroySwapchainKHR( m_vkLocalDevice, swapChainCreateInfo.oldSwapchain, &m_vkAllocationCallbacks );
        }

        m_swapchainData.swapchainImageCount = 0;
        HR( vkGetSwapchainImagesKHR( m_vkLocalDevice, m_swapchainData.vkSwapchain, &m_swapchainData.swapchainImageCount, VK_NULL_HANDLE ) );

        m_swapchainData.swapchainImageCount = zp_min( m_swapchainData.swapchainImageCount, static_cast<zp_uint32_t>( m_swapchainData.swapchainImages.length() ) );
        HR( vkGetSwapchainImagesKHR( m_vkLocalDevice, m_swapchainData.vkSwapchain, &m_swapchainData.swapchainImageCount, m_swapchainData.swapchainImages.data() ) );

        {
            if( m_swapchainData.vkSwapchainDefaultRenderPass )
            {
                const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 1, false );

                m_delayedDestroy[ index ] = {
                    .frameIndex = m_currentFrameIndex,
                    .handle = m_swapchainData.vkSwapchainDefaultRenderPass,
                    .allocator = &m_vkAllocationCallbacks,
                    .localDevice = m_vkLocalDevice,
                    .instance = m_vkInstance,
                    .order = index,
                    .type = DelayedDestroyType::RenderPass,
                };

                m_swapchainData.vkSwapchainDefaultRenderPass = VK_NULL_HANDLE;
            }

            VkAttachmentDescription colorAttachments[] {
                {
                    .format = m_swapchainData.vkSwapChainFormat,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                }
            };

            VkAttachmentReference colorAttachmentRefs[] {
                {
                    .attachment = 0,
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                }
            };

            VkSubpassDescription subPassDescriptions[] {
                {
                    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                    .colorAttachmentCount = ZP_ARRAY_SIZE( colorAttachmentRefs ),
                    .pColorAttachments = colorAttachmentRefs,
                }
            };

            VkSubpassDependency subPassDependencies[] {
                {
                    .srcSubpass = VK_SUBPASS_EXTERNAL,
                    .dstSubpass = 0,
                    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                }
            };

            VkRenderPassCreateInfo renderPassInfo {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .attachmentCount = ZP_ARRAY_SIZE( colorAttachments ),
                .pAttachments = colorAttachments,
                .subpassCount = ZP_ARRAY_SIZE( subPassDescriptions ),
                .pSubpasses = subPassDescriptions,
                .dependencyCount = ZP_ARRAY_SIZE( subPassDependencies ),
                .pDependencies = subPassDependencies,
            };

            HR( vkCreateRenderPass( m_vkLocalDevice, &renderPassInfo, &m_vkAllocationCallbacks, &m_swapchainData.vkSwapchainDefaultRenderPass ) );

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_RENDER_PASS, m_swapchainData.vkSwapchainDefaultRenderPass, "Swapchain Default Render Pass" );
        }

        for( zp_size_t i = 0; i < m_swapchainData.swapchainImageCount; ++i )
        {
            VkImageViewCreateInfo imageViewCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = m_swapchainData.swapchainImages[ i ],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_swapchainData.vkSwapChainFormat,
                .components {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                }
            };

            if( m_swapchainData.swapchainImageViews[ i ] != VK_NULL_HANDLE )
            {
                const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 1, false );

                m_delayedDestroy[ index ] = {
                    .frameIndex = m_currentFrameIndex,
                    .handle = m_swapchainData.swapchainImageViews[ i ],
                    .allocator = &m_vkAllocationCallbacks,
                    .localDevice = m_vkLocalDevice,
                    .instance = m_vkInstance,
                    .order = index,
                    .type = DelayedDestroyType::ImageView,
                };

                m_swapchainData.swapchainImageViews[ i ] = VK_NULL_HANDLE;
            };

            HR( vkCreateImageView( m_vkLocalDevice, &imageViewCreateInfo, &m_vkAllocationCallbacks, &m_swapchainData.swapchainImageViews[ i ] ) );

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_IMAGE_VIEW, m_swapchainData.swapchainImageViews[ i ], "Swapchain Image View %d", i );

            VkImageView attachments[] { m_swapchainData.swapchainImageViews[ i ] };

            VkFramebufferCreateInfo framebufferCreateInfo {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = m_swapchainData.vkSwapchainDefaultRenderPass,
                .attachmentCount = ZP_ARRAY_SIZE( attachments ),
                .pAttachments = attachments,
                .width = m_swapchainData.m_vkSwapchainExtent.width,
                .height = m_swapchainData.m_vkSwapchainExtent.height,
                .layers = 1,
            };

            if( m_swapchainData.swapchainFrameBuffers[ i ] != VK_NULL_HANDLE )
            {
                const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 1, false );

                m_delayedDestroy[ index ] = {
                    .frameIndex = m_currentFrameIndex,
                    .handle = m_swapchainData.swapchainFrameBuffers[ i ],
                    .allocator = &m_vkAllocationCallbacks,
                    .localDevice = m_vkLocalDevice,
                    .instance = m_vkInstance,
                    .order = index,
                    .type = DelayedDestroyType::FrameBuffer,
                };

                m_swapchainData.swapchainFrameBuffers[ i ] = VK_NULL_HANDLE;
            }

            HR( vkCreateFramebuffer( m_vkLocalDevice, &framebufferCreateInfo, &m_vkAllocationCallbacks, &m_swapchainData.swapchainFrameBuffers[ i ] ) );

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_FRAMEBUFFER, m_swapchainData.swapchainFrameBuffers[ i ], "Swapchain Framebuffer %d", i );
        }
    }

    void VulkanGraphicsDevice::resizeSwapchain( zp_uint32_t width, zp_uint32_t height )
    {
    }

    void VulkanGraphicsDevice::destroySwapchain()
    {
        for( zp_size_t i = 0; i < m_swapchainData.swapchainImageCount; ++i )
        {
            VkFramebuffer swapChainFramebuffer = m_swapchainData.swapchainFrameBuffers[ i ];
            vkDestroyFramebuffer( m_vkLocalDevice, swapChainFramebuffer, &m_vkAllocationCallbacks );
        }

        vkDestroyRenderPass( m_vkLocalDevice, m_swapchainData.vkSwapchainDefaultRenderPass, &m_vkAllocationCallbacks );
        m_swapchainData.vkSwapchainDefaultRenderPass = {};

        for( zp_size_t i = 0; i < m_swapchainData.swapchainImageCount; ++i )
        {
            VkImageView swapChainImageView = m_swapchainData.swapchainImageViews[ i ];
            vkDestroyImageView( m_vkLocalDevice, swapChainImageView, &m_vkAllocationCallbacks );
        }

        vkDestroySwapchainKHR( m_vkLocalDevice, m_swapchainData.vkSwapchain, &m_vkAllocationCallbacks );
        m_swapchainData.vkSwapchain = VK_NULL_HANDLE;
    }

    void VulkanGraphicsDevice::createPerFrameData()
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkFenceCreateInfo fenceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

#if ZP_USE_PROFILER
        VkQueryPoolCreateInfo pipelineStatsQueryPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
            .queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS,
            .queryCount = 8,
            .pipelineStatistics =
            VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT,
        };

        VkQueryPoolCreateInfo timestampQueryPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
            .queryType = VK_QUERY_TYPE_TIMESTAMP,
            .queryCount = 16,
        };
#endif

        zp_size_t perFrameStagingBufferOffset = 0;
        const zp_size_t perFrameStagingBufferSize = m_stagingBuffer.size / kBufferedFrameCount;

        for( zp_size_t i = 0; i < kBufferedFrameCount; ++i )
        {
            PerFrameData& perFrameData = m_perFrameData[ i ];
            HR( vkCreateSemaphore( m_vkLocalDevice, &semaphoreCreateInfo, &m_vkAllocationCallbacks, &perFrameData.vkSwapChainAcquireSemaphore ) );
            HR( vkCreateSemaphore( m_vkLocalDevice, &semaphoreCreateInfo, &m_vkAllocationCallbacks, &perFrameData.vkRenderFinishedSemaphore ) );
            HR( vkCreateFence( m_vkLocalDevice, &fenceCreateInfo, &m_vkAllocationCallbacks, &perFrameData.vkInFlightFence ) );
            HR( vkCreateFence( m_vkLocalDevice, &fenceCreateInfo, &m_vkAllocationCallbacks, &perFrameData.vkSwapChainImageAcquiredFence ) );
#if ZP_USE_PROFILER
            HR( vkCreateQueryPool( m_vkLocalDevice, &pipelineStatsQueryPoolCreateInfo, &m_vkAllocationCallbacks, &perFrameData.vkPipelineStatisticsQueryPool ) );
            HR( vkCreateQueryPool( m_vkLocalDevice, &timestampQueryPoolCreateInfo, &m_vkAllocationCallbacks, &perFrameData.vkTimestampQueryPool ) );
#endif
            perFrameData.perFrameStagingBuffer.graphicsBuffer = m_stagingBuffer.splitBuffer( perFrameStagingBufferOffset, perFrameStagingBufferSize );
            perFrameData.perFrameStagingBuffer.allocated = 0;

            perFrameData.commandQueueCount = 0;
            perFrameData.commandQueueCapacity = 0;
            perFrameData.commandQueues = nullptr;

            perFrameStagingBufferOffset += perFrameStagingBufferSize;

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_SEMAPHORE, perFrameData.vkSwapChainAcquireSemaphore, "Swapchain Acquire Semaphore %d", i );
            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_SEMAPHORE, perFrameData.vkRenderFinishedSemaphore, "Render Finished Semaphore %d", i );
            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_FENCE, perFrameData.vkInFlightFence, "In Flight Fence %d", i );
            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_FENCE, perFrameData.vkSwapChainImageAcquiredFence, "Swapchain Image Acquire Fence %d", i );
        }
    }

    void VulkanGraphicsDevice::destroyPerFrameData()
    {
        for( PerFrameData& perFrameData : m_perFrameData )
        {
            vkDestroySemaphore( m_vkLocalDevice, perFrameData.vkSwapChainAcquireSemaphore, &m_vkAllocationCallbacks );
            vkDestroySemaphore( m_vkLocalDevice, perFrameData.vkRenderFinishedSemaphore, &m_vkAllocationCallbacks );
            vkDestroyFence( m_vkLocalDevice, perFrameData.vkInFlightFence, &m_vkAllocationCallbacks );
            vkDestroyFence( m_vkLocalDevice, perFrameData.vkSwapChainImageAcquiredFence, &m_vkAllocationCallbacks );

#if ZP_USE_PROFILER
            vkDestroyQueryPool( m_vkLocalDevice, perFrameData.vkPipelineStatisticsQueryPool, &m_vkAllocationCallbacks );
            vkDestroyQueryPool( m_vkLocalDevice, perFrameData.vkTimestampQueryPool, &m_vkAllocationCallbacks );
#endif
            if( perFrameData.commandQueues )
            {
                for( zp_size_t i = 0; i < perFrameData.commandQueueCapacity; ++i )
                {
                    if( perFrameData.commandQueues && perFrameData.commandQueues[ i ].commandBuffer )
                    {
                        const auto commandBuffer = static_cast<VkCommandBuffer>( perFrameData.commandQueues[ i ].commandBuffer );
                        const auto commandPool = static_cast<VkCommandPool>( perFrameData.commandQueues[ i ].commandBufferPool );
                        vkFreeCommandBuffers( m_vkLocalDevice, commandPool, 1, &commandBuffer );
                    }
                }

                ZP_FREE( memoryLabel, perFrameData.commandQueues );
            }

            perFrameData.vkSwapChainAcquireSemaphore = VK_NULL_HANDLE;
            perFrameData.vkRenderFinishedSemaphore = VK_NULL_HANDLE;
            perFrameData.vkInFlightFence = {};
            perFrameData.vkSwapChainImageAcquiredFence = VK_NULL_HANDLE;
            perFrameData.swapChainImageIndex = 0;
#if ZP_USE_PROFILER
            perFrameData.vkPipelineStatisticsQueryPool = VK_NULL_HANDLE;
            perFrameData.vkTimestampQueryPool = VK_NULL_HANDLE;
#endif
            perFrameData.perFrameStagingBuffer = {};
            perFrameData.commandQueueCount = 0;
            perFrameData.commandQueueCapacity = 0;
            perFrameData.commandQueues = nullptr;
        }
    }

    void VulkanGraphicsDevice::beginFrame()
    {
        ZP_PROFILE_CPU_BLOCK();

        PerFrameData& currentFrameData = getCurrentFrameData();

        HR( vkWaitForFences( m_vkLocalDevice, 1, &currentFrameData.vkInFlightFence, VK_TRUE, UINT64_MAX ) );

        processDelayedDestroy();

        for( zp_size_t i = 0; i < currentFrameData.commandQueueCount; ++i )
        {
            HR( vkResetCommandBuffer( static_cast<VkCommandBuffer>( currentFrameData.commandQueues[ i ].commandBuffer ), 0 ) );
        }

        const VkResult result = vkAcquireNextImageKHR( m_vkLocalDevice, m_swapchainData.vkSwapchain, UINT64_MAX, currentFrameData.vkSwapChainAcquireSemaphore, VK_NULL_HANDLE, &currentFrameData.swapChainImageIndex );
        if( result != VK_SUCCESS )
        {
            if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR )
            {
                rebuildSwapchain();
            }

            ZP_INVALID_CODE_PATH();
        }

        HR( vkResetFences( m_vkLocalDevice, 1, &currentFrameData.vkInFlightFence ) );

#if ZP_USE_PROFILER && false
        {
            PerFrameData& prevFrameData = getFrameData( prevFrame );

            if( prevFrameData.commandQueueCount > 0 )
            {
                zp_uint64_t timestamps[16] {};
                VkResult r;

                r = vkGetQueryPoolResults( m_vkLocalDevice, prevFrameData.vkTimestampQueryPool, 0, prevFrameData.commandQueueCount * 2, sizeof( timestamps ), timestamps, sizeof( zp_uint64_t ), VK_QUERY_RESULT_64_BIT );
                if( r == VK_SUCCESS )
                {
                    zp_uint64_t totalTime = 0;

                    for( zp_int32_t i = 0; i < ( prevFrameData.commandQueueCount * 2 ); i += 2 )
                    {
                        totalTime += timestamps[ i + 1 ] - timestamps[ i ];
                    }

                    ZP_PROFILE_GPU_MARK( totalTime );
                }
            }
        }
#endif

        currentFrameData.perFrameStagingBuffer.allocated = 0;
        currentFrameData.commandQueueCount = 0;
    }

    void VulkanGraphicsDevice::submit()
    {
        ZP_PROFILE_CPU_BLOCK();

        const PerFrameData& currentFrameData = getCurrentFrameData();

        // if there are no command queues, add the default swapchain render pass
        if( currentFrameData.commandQueueCount == 0 )
        {
            auto cmd = requestCommandQueue( ZP_RENDER_QUEUE_GRAPHICS );

            beginRenderPass( nullptr, cmd );

            endRenderPass( cmd );
        }

        // release any open command queues
        if( currentFrameData.commandQueueCount > 0 )
        {
            const zp_size_t preCommandQueueIndex = currentFrameData.commandQueueCount - 1;
            releaseCommandQueue( currentFrameData.commandQueues + preCommandQueueIndex );

#if ZP_USE_PROFILER
            //vkCmdWriteTimestamp( buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, currentFrameData.vkTimestampQueryPool, preCommandQueueIndex * 2 + 1 );
            //vkCmdEndQuery( buffer, currentFrameData.vkPipelineStatisticsQueryPool, 0 );
            //vkCmdResetQueryPool( static_cast<VkCommandBuffer>( currentFrameData.commandQueues[ preCommandQueueIndex ].commandBuffer ), currentFrameData.vkTimestampQueryPool, commandQueueCount * 2 + 2, 16 - (commandQueueCount * 2 + 2));
#endif
            //HR( vkEndCommandBuffer( buffer ) );
        }

        const VkPipelineStageFlags waitStages[] { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        const VkSemaphore waitSemaphores[] { currentFrameData.vkSwapChainAcquireSemaphore };
        ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( waitStages ) == ZP_ARRAY_SIZE( waitSemaphores ) );

        VkCommandBuffer graphicsQueueCommandBuffers[currentFrameData.commandQueueCount];
        for( zp_size_t i = 0; i < currentFrameData.commandQueueCount; ++i )
        {
            graphicsQueueCommandBuffers[ i ] = static_cast<VkCommandBuffer>( currentFrameData.commandQueues[ i ].commandBuffer );
        }

        VkSemaphore signalSemaphores[] { currentFrameData.vkRenderFinishedSemaphore };
        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = ZP_ARRAY_SIZE( waitSemaphores ),
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = static_cast<uint32_t>( currentFrameData.commandQueueCount ),
            .pCommandBuffers = graphicsQueueCommandBuffers,
            .signalSemaphoreCount = ZP_ARRAY_SIZE( signalSemaphores ),
            .pSignalSemaphores = signalSemaphores,
        };

        HR( vkQueueSubmit( m_vkRenderQueues[ ZP_RENDER_QUEUE_GRAPHICS ], 1, &submitInfo, currentFrameData.vkInFlightFence ) );
    }

    void VulkanGraphicsDevice::present()
    {
        ZP_PROFILE_CPU_BLOCK();

        const PerFrameData& currentFrameData = getCurrentFrameData();

        const VkSemaphore waitSemaphores[] { currentFrameData.vkRenderFinishedSemaphore };

        const VkSwapchainKHR swapchains[] { m_swapchainData.vkSwapchain };
        const uint32_t imageIndices[] { currentFrameData.swapChainImageIndex };
        ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( swapchains ) == ZP_ARRAY_SIZE( imageIndices ) );

        const VkPresentInfoKHR presentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = ZP_ARRAY_SIZE( waitSemaphores ),
            .pWaitSemaphores = waitSemaphores,
            .swapchainCount = ZP_ARRAY_SIZE( swapchains ),
            .pSwapchains = swapchains,
            .pImageIndices = imageIndices,
        };

        const VkResult result = vkQueuePresentKHR( m_vkRenderQueues[ ZP_RENDER_QUEUE_PRESENT ], &presentInfo );
        if( result != VK_SUCCESS )
        {
            if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR )
            {
                rebuildSwapchain();
            }
            else
            {
                ZP_INVALID_CODE_PATH();
            }
        }

        ++m_currentFrameIndex;
    }

    void VulkanGraphicsDevice::waitForGPU()
    {
        ZP_PROFILE_CPU_BLOCK();

        HR( vkDeviceWaitIdle( m_vkLocalDevice ) );
    }

    void VulkanGraphicsDevice::createRenderPass( const RenderPassDesc* renderPassDesc, RenderPass* renderPass )
    {
        VkAttachmentDescription attachmentDescriptions[] {
            {
                .flags = 0,
                .format = m_swapchainData.vkSwapChainFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            }
        };

        VkAttachmentReference colorRef {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
        };

        VkSubpassDescription subPass {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorRef,
        };

        VkSubpassDependency subPassDependency {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        };

        VkRenderPassCreateInfo renderPassCreateInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = ZP_ARRAY_SIZE( attachmentDescriptions ),
            .pAttachments = attachmentDescriptions,
            .subpassCount = 1,
            .pSubpasses = &subPass,
            .dependencyCount = 1,
            .pDependencies = &subPassDependency,
        };

        VkRenderPass vkRenderPass;
        HR( vkCreateRenderPass( m_vkLocalDevice, &renderPassCreateInfo, &m_vkAllocationCallbacks, &vkRenderPass ) );

        renderPass->internalRenderPass = vkRenderPass;

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, renderPassDesc->name, VK_OBJECT_TYPE_RENDER_PASS, vkRenderPass );
    }

    void VulkanGraphicsDevice::destroyRenderPass( RenderPass* renderPass )
    {
        const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 1, false );

        m_delayedDestroy[ index ] = {
            .frameIndex = m_currentFrameIndex,
            .handle = renderPass->internalRenderPass,
            .allocator = &m_vkAllocationCallbacks,
            .localDevice = m_vkLocalDevice,
            .instance = m_vkInstance,
            .order = index,
            .type = DelayedDestroyType::RenderPass,
        };

        renderPass->internalRenderPass = nullptr;
    }

    void VulkanGraphicsDevice::createGraphicsPipeline( const GraphicsPipelineStateCreateDesc* graphicsPipelineStateCreateDesc, GraphicsPipelineState* graphicsPipelineState )
    {
        zp_bool_t vertexShaderUsed = false;
        zp_bool_t tessellationControlShaderUsed = false;
        zp_bool_t tessellationEvaluationShaderUsed = false;
        zp_bool_t geometryShaderUsed = false;
        zp_bool_t fragmentShaderUsed = false;
        zp_bool_t meshShaderUsed = false;
        zp_bool_t taskShaderUsed = false;

        VkPipelineShaderStageCreateInfo shaderStageCreateInfos[graphicsPipelineStateCreateDesc->shaderStageCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->shaderStageCount; ++i )
        {
            const ShaderResourceHandle& srcShaderStage = graphicsPipelineStateCreateDesc->shaderStages[ i ];
            const VkShaderStageFlagBits stage = Convert( srcShaderStage->shaderStage );

            shaderStageCreateInfos[ i ] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = stage,
                .module = static_cast<VkShaderModule>( srcShaderStage->shaderHandle ),
                .pName = srcShaderStage->entryPoint.c_str(),
                .pSpecializationInfo = nullptr,
            };

            vertexShaderUsed |= stage & VK_SHADER_STAGE_VERTEX_BIT;
            tessellationControlShaderUsed |= stage & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            tessellationEvaluationShaderUsed |= stage & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            geometryShaderUsed |= stage & VK_SHADER_STAGE_GEOMETRY_BIT;
            fragmentShaderUsed |= stage & VK_SHADER_STAGE_FRAGMENT_BIT;
            meshShaderUsed |= stage & VK_SHADER_STAGE_MESH_BIT_EXT;
            taskShaderUsed |= stage & VK_SHADER_STAGE_TASK_BIT_EXT;
        }

        VkVertexInputBindingDescription vertexInputBindingDescriptions[graphicsPipelineStateCreateDesc->vertexBindingCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->vertexBindingCount; ++i )
        {
            const VertexBinding& srcVertexBinding = graphicsPipelineStateCreateDesc->vertexBindings[ i ];

            vertexInputBindingDescriptions[ i ] = {
                .binding = srcVertexBinding.binding,
                .stride = srcVertexBinding.stride,
                .inputRate = Convert( srcVertexBinding.inputRate ),
            };
        }

        VkVertexInputAttributeDescription vertexInputAttributeDescriptions[graphicsPipelineStateCreateDesc->vertexAttributeCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->vertexAttributeCount; ++i )
        {
            const VertexAttribute& srcVertexAttribute = graphicsPipelineStateCreateDesc->vertexAttributes[ i ];

            vertexInputAttributeDescriptions[ i ] = {
                .location = srcVertexAttribute.location,
                .binding = srcVertexAttribute.binding,
                .format = Convert( srcVertexAttribute.format ),
                .offset = srcVertexAttribute.offset,
            };
        }

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = static_cast<uint32_t>( graphicsPipelineStateCreateDesc->vertexBindingCount ),
            .pVertexBindingDescriptions = vertexInputBindingDescriptions,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>( graphicsPipelineStateCreateDesc->vertexAttributeCount ),
            .pVertexAttributeDescriptions = vertexInputAttributeDescriptions,
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = Convert( graphicsPipelineStateCreateDesc->primitiveTopology ),
            .primitiveRestartEnable = graphicsPipelineStateCreateDesc->primitiveRestartEnable,
        };

        VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
            .patchControlPoints = graphicsPipelineStateCreateDesc->patchControlPoints,
        };

        VkViewport viewports[graphicsPipelineStateCreateDesc->viewportCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->viewportCount; ++i )
        {
            const Viewport& srcViewport = graphicsPipelineStateCreateDesc->viewports[ i ];

            viewports[ i ] = {
                .x = srcViewport.x,
                .y = srcViewport.y,
                .width = srcViewport.width,
                .height = srcViewport.height,
                .minDepth = srcViewport.minDepth,
                .maxDepth = srcViewport.maxDepth,
            };
        }

        VkRect2D scissorRects[graphicsPipelineStateCreateDesc->scissorRectCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->scissorRectCount; ++i )
        {
            const ScissorRect& srcScissor = graphicsPipelineStateCreateDesc->scissorRects[ i ];

            scissorRects[ i ] = {
                .offset = {
                    .x = srcScissor.x,
                    .y = srcScissor.y
                },
                .extent = {
                    .width = static_cast<uint32_t>(srcScissor.width),
                    .height = static_cast<uint32_t>(srcScissor.height),
                }
            };
        }

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = static_cast<uint32_t>( graphicsPipelineStateCreateDesc->viewportCount ),
            .pViewports = viewports,
            .scissorCount = static_cast<uint32_t>( graphicsPipelineStateCreateDesc->scissorRectCount ),
            .pScissors = scissorRects,
        };

        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = graphicsPipelineStateCreateDesc->depthClampEnabled,
            .rasterizerDiscardEnable = graphicsPipelineStateCreateDesc->rasterizerDiscardEnable,
            .polygonMode = Convert( graphicsPipelineStateCreateDesc->polygonFillMode ),
            .cullMode = Convert( graphicsPipelineStateCreateDesc->cullMode ),
            .frontFace = Convert( graphicsPipelineStateCreateDesc->frontFaceMode ),
            .depthBiasEnable = graphicsPipelineStateCreateDesc->depthBiasEnable,
            .depthBiasConstantFactor = graphicsPipelineStateCreateDesc->depthBiasConstantFactor,
            .depthBiasClamp = graphicsPipelineStateCreateDesc->depthBiasClamp,
            .depthBiasSlopeFactor = graphicsPipelineStateCreateDesc->depthBiasSlopeFactor,
            .lineWidth = graphicsPipelineStateCreateDesc->lineWidth,
        };

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = Convert( graphicsPipelineStateCreateDesc->sampleCount ),
            .sampleShadingEnable = graphicsPipelineStateCreateDesc->sampleShadingEnable,
            .minSampleShading = graphicsPipelineStateCreateDesc->minSampleShading,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = graphicsPipelineStateCreateDesc->alphaToCoverageEnable,
            .alphaToOneEnable = graphicsPipelineStateCreateDesc->alphaToOneEnable,
        };

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = graphicsPipelineStateCreateDesc->depthTestEnable,
            .depthWriteEnable = graphicsPipelineStateCreateDesc->depthWriteEnable,
            .depthCompareOp = Convert( graphicsPipelineStateCreateDesc->depthCompare ),
            .depthBoundsTestEnable = graphicsPipelineStateCreateDesc->depthBoundsTestEnable,
            .stencilTestEnable = graphicsPipelineStateCreateDesc->stencilTestEnable,
            .front {
                .failOp = Convert( graphicsPipelineStateCreateDesc->front.fail ),
                .passOp = Convert( graphicsPipelineStateCreateDesc->front.pass ),
                .depthFailOp = Convert( graphicsPipelineStateCreateDesc->front.depthFail ),
                .compareOp = Convert( graphicsPipelineStateCreateDesc->front.compare ),
                .compareMask = graphicsPipelineStateCreateDesc->front.compareMask,
                .writeMask = graphicsPipelineStateCreateDesc->front.writeMask,
                .reference = graphicsPipelineStateCreateDesc->front.reference,
            },
            .back {
                .failOp = Convert( graphicsPipelineStateCreateDesc->back.fail ),
                .passOp = Convert( graphicsPipelineStateCreateDesc->back.pass ),
                .depthFailOp = Convert( graphicsPipelineStateCreateDesc->back.depthFail ),
                .compareOp = Convert( graphicsPipelineStateCreateDesc->back.compare ),
                .compareMask = graphicsPipelineStateCreateDesc->back.compareMask,
                .writeMask = graphicsPipelineStateCreateDesc->back.writeMask,
                .reference = graphicsPipelineStateCreateDesc->back.reference,
            },
            .minDepthBounds = graphicsPipelineStateCreateDesc->minDepthBounds,
            .maxDepthBounds = graphicsPipelineStateCreateDesc->maxDepthBounds,
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[graphicsPipelineStateCreateDesc->blendStateCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->blendStateCount; ++i )
        {
            const BlendState& srcBlendState = graphicsPipelineStateCreateDesc->blendStates[ i ];

            colorBlendAttachmentStates[ i ] = {
                .blendEnable = srcBlendState.blendEnable,
                .srcColorBlendFactor = Convert( srcBlendState.srcColorBlendFactor ),
                .dstColorBlendFactor = Convert( srcBlendState.dstColorBlendFactor ),
                .colorBlendOp = Convert( srcBlendState.colorBlendOp ),
                .srcAlphaBlendFactor = Convert( srcBlendState.srcAlphaBlendFactor ),
                .dstAlphaBlendFactor = Convert( srcBlendState.dstAlphaBlendFactor ),
                .alphaBlendOp = Convert( srcBlendState.alphaBlendOp ),
                .colorWriteMask = Convert( srcBlendState.writeMask ),
            };
        }

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = graphicsPipelineStateCreateDesc->blendLogicalOpEnable,
            .logicOp = Convert( graphicsPipelineStateCreateDesc->blendLogicalOp ),
            .attachmentCount = static_cast<uint32_t>( graphicsPipelineStateCreateDesc->blendStateCount ),
            .pAttachments = colorBlendAttachmentStates,
            .blendConstants {
                graphicsPipelineStateCreateDesc->blendConstants[ 0 ],
                graphicsPipelineStateCreateDesc->blendConstants[ 1 ],
                graphicsPipelineStateCreateDesc->blendConstants[ 2 ],
                graphicsPipelineStateCreateDesc->blendConstants[ 3 ],
            }
        };

        const VkDynamicState dynamicStates[] {
            VK_DYNAMIC_STATE_CULL_MODE,
        };

        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = ZP_ARRAY_SIZE( dynamicStates ),
            .pDynamicStates = dynamicStates,
        };

        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = static_cast<uint32_t>( graphicsPipelineStateCreateDesc->shaderStageCount ),
            .pStages = shaderStageCreateInfos,
            .pVertexInputState = &vertexInputStateCreateInfo,
            .pInputAssemblyState = &inputAssemblyStateCreateInfo,
            .pTessellationState = &tessellationStateCreateInfo,
            .pViewportState = &viewportStateCreateInfo,
            .pRasterizationState = &rasterizationStateCreateInfo,
            .pMultisampleState = &multisampleStateCreateInfo,
            .pDepthStencilState = &depthStencilStateCreateInfo,
            .pColorBlendState = &colorBlendStateCreateInfo,
            .pDynamicState = nullptr, //&pipelineDynamicStateCreateInfo,
            .layout = static_cast<VkPipelineLayout>( graphicsPipelineStateCreateDesc->layout->layout ),
            .renderPass = static_cast<VkRenderPass>( graphicsPipelineStateCreateDesc->renderPass->internalRenderPass ),
            .subpass = graphicsPipelineStateCreateDesc->subPass,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
        };

        VkPipeline pipeline;
        HR( vkCreateGraphicsPipelines( m_vkLocalDevice, m_vkPipelineCache, 1, &graphicsPipelineCreateInfo, &m_vkAllocationCallbacks, &pipeline ) );
        graphicsPipelineState->pipelineState = pipeline;
        graphicsPipelineState->pipelineHash = zp_fnv128_1a( graphicsPipelineCreateInfo );

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, graphicsPipelineStateCreateDesc->name, VK_OBJECT_TYPE_PIPELINE, pipeline );
    }

    void VulkanGraphicsDevice::destroyGraphicsPipeline( GraphicsPipelineState* graphicsPipelineState )
    {
        const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 1, false );

        m_delayedDestroy[ index ] = {
            .frameIndex = m_currentFrameIndex,
            .handle = graphicsPipelineState->pipelineState,
            .allocator = &m_vkAllocationCallbacks,
            .localDevice = m_vkLocalDevice,
            .instance = m_vkInstance,
            .order = index,
            .type = DelayedDestroyType::Pipeline,
        };

        graphicsPipelineState->pipelineState = nullptr;
    }

    void VulkanGraphicsDevice::createPipelineLayout( const PipelineLayoutDesc* pipelineLayoutDesc, PipelineLayout* pipelineLayout )
    {
        VkSamplerCreateInfo samplerCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        };

        VkSampler sampler;
        vkCreateSampler( m_vkLocalDevice, &samplerCreateInfo, &m_vkAllocationCallbacks, &sampler );

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, "", VK_OBJECT_TYPE_SAMPLER, sampler );

        VkDescriptorSetLayoutBinding bindings[] = {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                .descriptorCount = 0,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            }
        };

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = 0,
            .bindingCount = ZP_ARRAY_SIZE( bindings ),
            .pBindings = bindings,
        };

        VkDescriptorSetLayout setLayout;
        HR( vkCreateDescriptorSetLayout( m_vkLocalDevice, &descriptorSetLayoutCreateInfo, &m_vkAllocationCallbacks, &setLayout ) );

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, "", VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, setLayout );

        VkDescriptorSetAllocateInfo alloc {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_vkDescriptorPool,
            .descriptorSetCount = 0,
            .pSetLayouts = &setLayout,
        };

        VkDescriptorSet descriptorSet;
        vkAllocateDescriptorSets( m_vkLocalDevice, &alloc, &descriptorSet );

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, "", VK_OBJECT_TYPE_DESCRIPTOR_SET, descriptorSet );

        VkPushConstantRange pushConstantRange[] = { {} };

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &setLayout,
            .pushConstantRangeCount = ZP_ARRAY_SIZE( pushConstantRange ),
            .pPushConstantRanges = pushConstantRange,
        };

        VkPipelineLayout layout;
        HR( vkCreatePipelineLayout( m_vkLocalDevice, &pipelineLayoutCreateInfo, &m_vkAllocationCallbacks, &layout ) );
        pipelineLayout->layout = layout;

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, pipelineLayoutDesc->name, VK_OBJECT_TYPE_PIPELINE_LAYOUT, layout );
    }

    void VulkanGraphicsDevice::destroyPipelineLayout( PipelineLayout* pipelineLayout )
    {
        const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 1, false );

        m_delayedDestroy[ index ] = {
            .frameIndex = m_currentFrameIndex,
            .handle = pipelineLayout->layout,
            .allocator = &m_vkAllocationCallbacks,
            .localDevice = m_vkLocalDevice,
            .instance = m_vkInstance,
            .order = index,
            .type = DelayedDestroyType::PipelineLayout
        };

        pipelineLayout->layout = nullptr;
    }

    void VulkanGraphicsDevice::createBuffer( const GraphicsBufferDesc& graphicsBufferDesc, GraphicsBuffer* graphicsBuffer )
    {
        VkBufferCreateInfo bufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .flags = 0,
            .size = graphicsBufferDesc.size,
            .usage = Convert( graphicsBufferDesc.usageFlags ),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
        };

        VkBuffer buffer;
        HR( vkCreateBuffer( m_vkLocalDevice, &bufferCreateInfo, &m_vkAllocationCallbacks, &buffer ) );

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements( m_vkLocalDevice, buffer, &memoryRequirements );

        VkMemoryAllocateInfo memoryAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = FindMemoryTypeIndex( m_vkPhysicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, Convert( graphicsBufferDesc.memoryPropertyFlags ) ),
        };

        VkDeviceMemory deviceMemory;
        HR( vkAllocateMemory( m_vkLocalDevice, &memoryAllocateInfo, &m_vkAllocationCallbacks, &deviceMemory ) );

        HR( vkBindBufferMemory( m_vkLocalDevice, buffer, deviceMemory, 0 ) );

        *graphicsBuffer = {
            .buffer = buffer,
            .deviceMemory = deviceMemory,
            .offset = 0,
            .size = memoryRequirements.size,
            .alignment = memoryRequirements.alignment,
            .usageFlags = graphicsBufferDesc.usageFlags,
            .isVirtualBuffer = false,
        };

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, graphicsBufferDesc.name, VK_OBJECT_TYPE_BUFFER, buffer );
    }

    void VulkanGraphicsDevice::destroyBuffer( GraphicsBuffer* graphicsBuffer )
    {
        if( !graphicsBuffer->isVirtualBuffer )
        {
            const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 2, false );

            m_delayedDestroy[ index + 0 ] = {
                .frameIndex = m_currentFrameIndex,
                .handle = graphicsBuffer->buffer,
                .allocator = &m_vkAllocationCallbacks,
                .localDevice = m_vkLocalDevice,
                .instance = m_vkInstance,
                .order = index + 0,
                .type = DelayedDestroyType::Buffer
            };

            m_delayedDestroy[ index + 1 ] = {
                .frameIndex = m_currentFrameIndex,
                .handle = graphicsBuffer->deviceMemory,
                .allocator = &m_vkAllocationCallbacks,
                .localDevice = m_vkLocalDevice,
                .instance = m_vkInstance,
                .order = index + 1,
                .type = DelayedDestroyType::Memory
            };
        }

        *graphicsBuffer = {};
    };

    void VulkanGraphicsDevice::createShader( const ShaderDesc& shaderDesc, Shader* shader )
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = shaderDesc.codeSizeInBytes, // codeSizeInBytes in bytes -> uint
            .pCode = static_cast<const uint32_t*>( shaderDesc.codeData ),
        };

        VkShaderModule shaderModule;
        HR( vkCreateShaderModule( m_vkLocalDevice, &shaderModuleCreateInfo, &m_vkAllocationCallbacks, &shaderModule ) );
        shader->shaderHandle = shaderModule;
        shader->shaderStage = shaderDesc.shaderStage;
        shader->shaderHash = zp_fnv128_1a( shaderDesc.codeData, shaderDesc.codeSizeInBytes );
        shader->entryPoint = shaderDesc.entryPointName;

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, shaderDesc.name, VK_OBJECT_TYPE_SHADER_MODULE, shaderModule );
    }

    void VulkanGraphicsDevice::destroyShader( Shader* shader )
    {
        const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 1, false );

        m_delayedDestroy[ index ] = {
            .frameIndex = m_currentFrameIndex,
            .handle = shader->shaderHandle,
            .allocator = &m_vkAllocationCallbacks,
            .localDevice = m_vkLocalDevice,
            .instance = m_vkInstance,
            .order = index,
            .type = DelayedDestroyType::Shader
        };

        *shader = {};
    }

    void VulkanGraphicsDevice::createTexture( const TextureCreateDesc* textureCreateDesc, Texture* texture )
    {
        VkImageCreateInfo imageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = Convert( textureCreateDesc->textureDimension ),
            .format = Convert( textureCreateDesc->textureFormat ),
            .extent {
                .width =  textureCreateDesc->size.width,
                .height = textureCreateDesc->size.height,
                .depth =  IsTextureArray( textureCreateDesc->textureDimension ) ? zp_max( textureCreateDesc->size.depth, 1u ) : 1,
            },
            .mipLevels = textureCreateDesc->mipCount,
            .arrayLayers = textureCreateDesc->arrayLayers,
            .samples = Convert( textureCreateDesc->samples ),
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = Convert( textureCreateDesc->usage ),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VkImage image;
        HR( vkCreateImage( m_vkLocalDevice, &imageCreateInfo, &m_vkAllocationCallbacks, &image ) );

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, "Image", VK_OBJECT_TYPE_IMAGE, image );

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements( m_vkLocalDevice, image, &memoryRequirements );

        VkMemoryAllocateInfo memoryAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = FindMemoryTypeIndex( m_vkPhysicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, Convert( textureCreateDesc->memoryPropertyFlags ) ),
        };

        VkDeviceMemory deviceMemory;
        HR( vkAllocateMemory( m_vkLocalDevice, &memoryAllocateInfo, &m_vkAllocationCallbacks, &deviceMemory ) );

        HR( vkBindImageMemory( m_vkLocalDevice, image, deviceMemory, 0 ) );

        VkImageViewCreateInfo imageViewCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = ConvertImageView( textureCreateDesc->textureDimension ),
            .format = imageCreateInfo.format,
            .components {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange {
                .aspectMask = ConvertAspect( textureCreateDesc->usage ),
                .baseMipLevel = 0,
                .levelCount = textureCreateDesc->mipCount,
                .baseArrayLayer = 0,
                .layerCount = textureCreateDesc->arrayLayers
            },
        };

        VkImageView imageView;
        HR( vkCreateImageView( m_vkLocalDevice, &imageViewCreateInfo, &m_vkAllocationCallbacks, &imageView ) );

        *texture = {
            .textureDimension = textureCreateDesc->textureDimension,
            .textureFormat = textureCreateDesc->textureFormat,
            .usage = textureCreateDesc->usage,
            .size = textureCreateDesc->size,
            .textureHandle = image,
            .textureViewHandle = imageView,
            .textureMemoryHandle = deviceMemory,
        };

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, "Image View", VK_OBJECT_TYPE_IMAGE_VIEW, imageView );
    }

    void VulkanGraphicsDevice::destroyTexture( Texture* texture )
    {
        const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 3, false );

        m_delayedDestroy[ index + 0 ] = {
            .frameIndex = m_currentFrameIndex,
            .handle = texture->textureViewHandle,
            .allocator = &m_vkAllocationCallbacks,
            .localDevice = m_vkLocalDevice,
            .instance = m_vkInstance,
            .order = index + 0,
            .type = DelayedDestroyType::ImageView,
        };
        m_delayedDestroy[ index + 1 ] = {
            .frameIndex = m_currentFrameIndex,
            .handle = texture->textureMemoryHandle,
            .allocator = &m_vkAllocationCallbacks,
            .localDevice = m_vkLocalDevice,
            .instance = m_vkInstance,
            .order = index + 1,
            .type = DelayedDestroyType::Memory,
        };
        m_delayedDestroy[ index + 2 ] = {
            .frameIndex = m_currentFrameIndex,
            .handle = texture->textureHandle,
            .allocator = &m_vkAllocationCallbacks,
            .localDevice = m_vkLocalDevice,
            .instance = m_vkInstance,
            .order = index + 2,
            .type = DelayedDestroyType::Image,
        };

        *texture = {};
    };

    void VulkanGraphicsDevice::createSampler( const SamplerCreateDesc* samplerCreateDesc, Sampler* sampler )
    {
        VkSamplerCreateInfo samplerCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = Convert( samplerCreateDesc->magFilter ),
            .minFilter = Convert( samplerCreateDesc->minFilter ),
            .mipmapMode = Convert( samplerCreateDesc->mipmapMode ),
            .addressModeU = Convert( samplerCreateDesc->addressModeU ),
            .addressModeV = Convert( samplerCreateDesc->addressModeV ),
            .addressModeW = Convert( samplerCreateDesc->addressModeW ),
            .mipLodBias = samplerCreateDesc->mipLodBias,
            .anisotropyEnable = samplerCreateDesc->anisotropyEnabled,
            .maxAnisotropy = samplerCreateDesc->maxAnisotropy,
            .compareEnable = samplerCreateDesc->compareEnabled,
            .compareOp = Convert( samplerCreateDesc->compareOp ),
            .minLod = samplerCreateDesc->minLod,
            .maxLod = samplerCreateDesc->maxLod,
            .borderColor = Convert( samplerCreateDesc->borderColor ),
            .unnormalizedCoordinates = samplerCreateDesc->unnormalizedCoordinates,
        };

        VkSampler vkSampler;
        HR( vkCreateSampler( m_vkLocalDevice, &samplerCreateInfo, &m_vkAllocationCallbacks, &vkSampler ) );

        *sampler = {
            .samplerHandle = vkSampler,
        };

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, "Sampler", VK_OBJECT_TYPE_SAMPLER, vkSampler );
    }

    void VulkanGraphicsDevice::destroySampler( Sampler* sampler )
    {
        const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 1, false );
        m_delayedDestroy[ index ] = {
            .frameIndex = m_currentFrameIndex,
            .handle = sampler->samplerHandle,
            .allocator = &m_vkAllocationCallbacks,
            .localDevice = m_vkLocalDevice,
            .instance = m_vkInstance,
            .order = index,
            .type = DelayedDestroyType::Sampler,
        };

        *sampler = {};
    };

    void VulkanGraphicsDevice::mapBuffer( zp_size_t offset, zp_size_t size, const GraphicsBuffer& graphicsBuffer, void** memory )
    {
        VkDeviceMemory deviceMemory = static_cast<VkDeviceMemory>( graphicsBuffer.deviceMemory );
        HR( vkMapMemory( m_vkLocalDevice, deviceMemory, offset + graphicsBuffer.offset, size, 0, memory ) );
    }

    void VulkanGraphicsDevice::unmapBuffer( const GraphicsBuffer& graphicsBuffer )
    {
        VkDeviceMemory deviceMemory = static_cast<VkDeviceMemory>( graphicsBuffer.deviceMemory );
        vkUnmapMemory( m_vkLocalDevice, deviceMemory );
    }

    CommandQueue* VulkanGraphicsDevice::requestCommandQueue( RenderQueue queue )
    {
        // TODO: remove when other queues are supported
        queue = ZP_RENDER_QUEUE_GRAPHICS;

        PerFrameData& frameData = getCurrentFrameData();

        const zp_size_t commandQueueIndex = Atomic::IncrementSizeT( &frameData.commandQueueCount ) - 1;
        if( commandQueueIndex == frameData.commandQueueCapacity )
        {
            const zp_size_t newCommandQueueCapacity = frameData.commandQueueCapacity == 0 ? 4 : frameData.commandQueueCapacity * 2;

            auto* newCommandQueues = ZP_MALLOC_T_ARRAY( memoryLabel, CommandQueue, newCommandQueueCapacity );
            zp_zero_memory_array( newCommandQueues, newCommandQueueCapacity );

            if( frameData.commandQueues )
            {
                for( zp_size_t i = 0; i < frameData.commandQueueCount; ++i )
                {
                    newCommandQueues[ i ] = zp_move( frameData.commandQueues[ i ] );
                }

                ZP_FREE( memoryLabel, frameData.commandQueues );
                frameData.commandQueues = nullptr;
            }

            frameData.commandQueues = newCommandQueues;
            frameData.commandQueueCapacity = newCommandQueueCapacity;
        }

        if( false )
        {
            const zp_size_t prevCommandQueueIndex = commandQueueIndex - 1;
            CommandQueue& prevCommandQueue = frameData.commandQueues[ prevCommandQueueIndex ];
            VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>( prevCommandQueue.commandBuffer );

            if( commandBuffer )
            {
#if ZP_USE_PROFILER
                if( prevCommandQueue.queue != ZP_RENDER_QUEUE_TRANSFER )
                {
                    //vkCmdWriteTimestamp( commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameData.vkTimestampQueryPool, prevCommandQueueIndex * 2 + 1 );
                    //vkCmdEndQuery( commandBuffer, frameData.vkPipelineStatisticsQueryPool, 0 );
                }
#endif
                HR( vkEndCommandBuffer( commandBuffer ) );
            }
        }

        CommandQueue* commandQueue = &frameData.commandQueues[ commandQueueIndex ];
        if( commandQueue->commandBuffer != VK_NULL_HANDLE && commandQueue->queue != queue )
        {
            auto commandBuffer = static_cast<VkCommandBuffer>( commandQueue->commandBuffer );
            auto commandPool = static_cast<VkCommandPool>( commandQueue->commandBufferPool );
            vkFreeCommandBuffers( m_vkLocalDevice, commandPool, 1, &commandBuffer );

            commandQueue->commandBuffer = nullptr;
            commandQueue->commandBufferPool = nullptr;
        }

        commandQueue->queue = queue;
        commandQueue->frameIndex = m_currentFrameIndex;

        if( commandQueue->commandBuffer == nullptr )
        {
            VkCommandPool commandPool = getCommandPool( commandQueue );
            commandQueue->commandBufferPool = commandPool;

            VkCommandBufferAllocateInfo commandBufferAllocateInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = commandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };

            VkCommandBuffer commandBuffer;
            HR( vkAllocateCommandBuffers( m_vkLocalDevice, &commandBufferAllocateInfo, &commandBuffer ) );
            commandQueue->commandBuffer = commandBuffer;

#if ZP_DEBUG
            const char* kQueueNames[] {
                "Graphics",
                "Transfer",
                "Compute",
                "Present",
            };

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_COMMAND_BUFFER, commandBuffer, "%s Command Buffer #%d (%d)", kQueueNames[ queue ], commandQueueIndex, Platform::GetCurrentThreadId() );
#endif
        }

        VkCommandBufferBeginInfo commandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };

        VkCommandBuffer buffer = static_cast<VkCommandBuffer>( commandQueue->commandBuffer );
        HR( vkBeginCommandBuffer( buffer, &commandBufferBeginInfo ) );

#if ZP_USE_PROFILER
        if( queue != ZP_RENDER_QUEUE_TRANSFER )
        {
            //vkCmdResetQueryPool( buffer, frameData.vkTimestampQueryPool, commandQueueIndex * 2, 2 );
            //vkCmdWriteTimestamp( buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameData.vkTimestampQueryPool, commandQueueIndex * 2 );
        }

        //vkCmdBeginQuery( buffer, frameData.vkPipelineStatisticsQueryPool, 0, 0 );
#endif

        return commandQueue;
    }

    CommandQueue* VulkanGraphicsDevice::requestCommandQueue( CommandQueue* parentCommandQueue )
    {
        return nullptr;
    }

    void VulkanGraphicsDevice::releaseCommandQueue( CommandQueue* commandQueue )
    {
        if( commandQueue )
        {
            VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>( commandQueue->commandBuffer );
            HR( vkEndCommandBuffer( commandBuffer ) );
        }
    }

    void VulkanGraphicsDevice::beginRenderPass( const RenderPass* renderPass, CommandQueue* commandQueue )
    {
        VkClearValue clearValues[] {
            {
                .color { 0, 0, 0, 1 },
            }
        };

        PerFrameData& frameData = getFrameData( commandQueue->frameIndex );
        VkRenderPassBeginInfo renderPassBeginInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass ? static_cast<VkRenderPass>( renderPass->internalRenderPass ) : m_swapchainData.vkSwapchainDefaultRenderPass,
            .framebuffer = m_swapchainData.swapchainFrameBuffers[ frameData.swapChainImageIndex ],
            .renderArea {
                .offset { 0, 0 },
                .extent { m_swapchainData.m_vkSwapchainExtent },
            },
            .clearValueCount = ZP_ARRAY_SIZE( clearValues ),
            .pClearValues = clearValues,
        };

        vkCmdBeginRenderPass( static_cast<VkCommandBuffer>(commandQueue->commandBuffer), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
    }

    void VulkanGraphicsDevice::nextSubpass( CommandQueue* commandQueue )
    {
        vkCmdNextSubpass( static_cast<VkCommandBuffer>(commandQueue->commandBuffer), VK_SUBPASS_CONTENTS_INLINE );
    }

    void VulkanGraphicsDevice::endRenderPass( CommandQueue* commandQueue )
    {
        vkCmdEndRenderPass( static_cast<VkCommandBuffer>(commandQueue->commandBuffer) );
    }

    void VulkanGraphicsDevice::bindPipeline( const GraphicsPipelineState* graphicsPipelineState, PipelineBindPoint bindPoint, CommandQueue* commandQueue )
    {
        vkCmdBindPipeline( static_cast<VkCommandBuffer>(commandQueue->commandBuffer), Convert( bindPoint ), static_cast<VkPipeline>(graphicsPipelineState->pipelineState) );
    }

    void VulkanGraphicsDevice::bindIndexBuffer( const GraphicsBuffer* graphicsBuffer, const IndexBufferFormat indexBufferFormat, zp_size_t offset, CommandQueue* commandQueue )
    {
        ZP_ASSERT( graphicsBuffer->usageFlags & ZP_GRAPHICS_BUFFER_USAGE_INDEX_BUFFER );

        const VkIndexType indexType = Convert( indexBufferFormat );
        const VkDeviceSize bufferOffset = graphicsBuffer->offset + offset;
        vkCmdBindIndexBuffer( static_cast<VkCommandBuffer>(commandQueue->commandBuffer), static_cast<VkBuffer>( graphicsBuffer->buffer), bufferOffset, indexType );
    }

    void VulkanGraphicsDevice::bindVertexBuffers( zp_uint32_t firstBinding, zp_uint32_t bindingCount, const GraphicsBuffer** graphicsBuffers, zp_size_t* offsets, CommandQueue* commandQueue )
    {
        VkBuffer vertexBuffers[bindingCount];
        VkDeviceSize vertexBufferOffsets[bindingCount];

        for( zp_uint32_t i = 0; i < bindingCount; ++i )
        {
            vertexBuffers[ i ] = static_cast<VkBuffer>( graphicsBuffers[ i ]->buffer );
            vertexBufferOffsets[ i ] = graphicsBuffers[ i ]->offset + ( offsets == nullptr ? 0 : offsets[ i ] );
        }

        vkCmdBindVertexBuffers( static_cast<VkCommandBuffer>(commandQueue->commandBuffer), firstBinding, bindingCount, vertexBuffers, vertexBufferOffsets );
    }

    void VulkanGraphicsDevice::updateTexture( const TextureUpdateDesc* textureUpdateDesc, const Texture* dstTexture, CommandQueue* commandQueue )
    {
        PerFrameData& frameData = getFrameData( commandQueue->frameIndex );
        GraphicsBufferAllocation allocation = frameData.perFrameStagingBuffer.allocate( textureUpdateDesc->textureDataSize );

        auto commandBuffer = static_cast<VkCommandBuffer>( commandQueue->commandBuffer );
        const zp_uint32_t mipCount = textureUpdateDesc->maxMipLevel - textureUpdateDesc->minMipLevel;

        // copy data to staging buffer
        {
            auto deviceMemory = static_cast<VkDeviceMemory>( allocation.deviceMemory );

            void* dstMemory {};
            HR( vkMapMemory( m_vkLocalDevice, deviceMemory, allocation.offset, allocation.size, 0, &dstMemory ) );

            zp_memcpy( dstMemory, allocation.size, textureUpdateDesc->textureData, textureUpdateDesc->textureDataSize );

            vkUnmapMemory( m_vkLocalDevice, deviceMemory );
        }

        // transfer from CPU to GPU memory
        {
            VkBufferMemoryBarrier memoryBarrier {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .buffer = static_cast<VkBuffer>( allocation.buffer ),
                .offset = allocation.offset,
                .size = allocation.size,
            };

            VkImageMemoryBarrier imageMemoryBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .image = static_cast<VkImage>( dstTexture->textureHandle ),
                .subresourceRange {
                    .aspectMask = ConvertAspect( dstTexture->usage ),
                    .baseMipLevel = textureUpdateDesc->minMipLevel,
                    .levelCount = mipCount,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                0,
                0,
                nullptr,
                1,
                &memoryBarrier,
                1,
                &imageMemoryBarrier
            );
        }

        // copy buffer to texture
        {
            zp_uint32_t mip = textureUpdateDesc->minMipLevel;

            VkBufferImageCopy imageCopies[mipCount];
            for( zp_uint32_t i = 0; i < mipCount; ++i, ++mip )
            {
                VkBufferImageCopy& imageCopy = imageCopies[ i ];
                const TextureMip& textureMip = textureUpdateDesc->mipLevels[ i ];

                imageCopy.bufferOffset = textureMip.offset;
                imageCopy.bufferRowLength = textureMip.width;
                imageCopy.bufferImageHeight = textureMip.height;
                imageCopy.imageSubresource = {
                    .aspectMask = ConvertAspect( dstTexture->usage ),
                    .mipLevel = mip,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                };
                imageCopy.imageOffset = {};
                imageCopy.imageExtent = { textureMip.width, textureMip.height, 1 };
            }

            vkCmdCopyBufferToImage(
                commandBuffer,
                static_cast<VkBuffer>( allocation.buffer ),
                static_cast<VkImage>( dstTexture->textureHandle ),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                mipCount,
                imageCopies
            );
        }

        // finalize image upload
        {
            VkImageMemoryBarrier imageMemoryBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .image = static_cast<VkImage>( dstTexture->textureHandle ),
                .subresourceRange {
                    .aspectMask = ConvertAspect( dstTexture->usage ),
                    .baseMipLevel = textureUpdateDesc->minMipLevel,
                    .levelCount = mipCount,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageMemoryBarrier
            );
        }
    }

    void VulkanGraphicsDevice::updateBuffer( const GraphicsBufferUpdateDesc* graphicsBufferUpdateDesc, const GraphicsBuffer* dstGraphicsBuffer, CommandQueue* commandQueue )
    {
        PerFrameData& frameData = getFrameData( commandQueue->frameIndex );
        GraphicsBufferAllocation allocation = frameData.perFrameStagingBuffer.allocate( graphicsBufferUpdateDesc->size );

        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>( commandQueue->commandBuffer );

        // copy data to staging buffer
        {
            VkDeviceMemory deviceMemory = static_cast<VkDeviceMemory>( allocation.deviceMemory );

            void* dstMemory {};
            HR( vkMapMemory( m_vkLocalDevice, deviceMemory, allocation.offset, allocation.size, 0, &dstMemory ) );

            zp_memcpy( dstMemory, allocation.size, graphicsBufferUpdateDesc->data, graphicsBufferUpdateDesc->size );

            VkMappedMemoryRange flushMemoryRange {
                .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                .memory = static_cast<VkDeviceMemory>(allocation.deviceMemory),
                .offset = allocation.offset,
                .size = allocation.size,
            };
            vkFlushMappedMemoryRanges( m_vkLocalDevice, 1, &flushMemoryRange );

            vkUnmapMemory( m_vkLocalDevice, deviceMemory );
        }

        // transfer from CPU to GPU memory
        if( false )
        {
            VkBufferMemoryBarrier memoryBarrier[2] {
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                    .buffer = static_cast<VkBuffer>( allocation.buffer ),
                    .offset = allocation.offset,
                    .size = allocation.size,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .buffer = static_cast<VkBuffer>( dstGraphicsBuffer->buffer ),
                    .offset = dstGraphicsBuffer->offset,
                    .size = allocation.size,

                }
            };

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                0,
                0,
                nullptr,
                ZP_ARRAY_SIZE( memoryBarrier ),
                memoryBarrier,
                0,
                nullptr
            );
        }

        // copy staging buffer to dst buffer
        {
            VkBufferCopy bufferCopy {
                .srcOffset = allocation.offset,
                .dstOffset = dstGraphicsBuffer->offset,
                .size = allocation.size,
            };

            vkCmdCopyBuffer(
                commandBuffer,
                static_cast<VkBuffer>( allocation.buffer ),
                static_cast<VkBuffer>( dstGraphicsBuffer->buffer ),
                1,
                &bufferCopy
            );
        }

        // finalize buffer upload
        {
            VkBufferMemoryBarrier memoryBarrier {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = static_cast<VkBuffer>( dstGraphicsBuffer->buffer ),
                .offset = dstGraphicsBuffer->offset,
                .size = allocation.size,
            };

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                1,
                &memoryBarrier,
                0,
                nullptr
            );
        }
    }

    void VulkanGraphicsDevice::draw( zp_uint32_t vertexCount, zp_uint32_t instanceCount, zp_uint32_t firstVertex, zp_uint32_t firstInstance, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        vkCmdDraw( commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );
    }

    void VulkanGraphicsDevice::drawIndirect( const GraphicsBuffer& buffer, zp_uint32_t drawCount, zp_uint32_t stride, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        VkBuffer indirectBuffer = static_cast<VkBuffer>(buffer.buffer);
        vkCmdDrawIndirect( commandBuffer, indirectBuffer, buffer.offset, drawCount, stride );
    }

    void VulkanGraphicsDevice::drawIndexed( zp_uint32_t indexCount, zp_uint32_t instanceCount, zp_uint32_t firstIndex, zp_int32_t vertexOffset, zp_uint32_t firstInstance, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        vkCmdDrawIndexed( commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
    }

    void VulkanGraphicsDevice::drawIndexedIndirect( const GraphicsBuffer& buffer, zp_uint32_t drawCount, zp_uint32_t stride, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        VkBuffer indirectBuffer = static_cast<VkBuffer>(buffer.buffer);
        vkCmdDrawIndexedIndirect( commandBuffer, indirectBuffer, buffer.offset, drawCount, stride );
    }

    void VulkanGraphicsDevice::dispatch( zp_uint32_t groupCountX, zp_uint32_t groupCountY, zp_uint32_t groupCountZ, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        vkCmdDispatch( commandBuffer, groupCountX, groupCountY, groupCountZ );
    }

    void VulkanGraphicsDevice::dispatchIndirect( const GraphicsBuffer& buffer, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        VkBuffer indirectBuffer = static_cast<VkBuffer>(buffer.buffer);
        vkCmdDispatchIndirect( commandBuffer, indirectBuffer, buffer.offset );
    }

    void VulkanGraphicsDevice::beginEventLabel( const char* eventLabel, const Color& color, CommandQueue* commandQueue )
    {
#if ZP_DEBUG
        VkDebugMarkerMarkerInfoEXT debugMarkerMarkerInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,
            .pMarkerName = eventLabel,
            .color {
                color.r,
                color.g,
                color.b,
                color.a,
            },
        };

        CallDebugUtil( vkCmdDebugMarkerBeginEXT, m_vkInstance, static_cast<VkCommandBuffer>(commandQueue->commandBuffer), &debugMarkerMarkerInfo );
#endif
    }

    void VulkanGraphicsDevice::endEventLabel( CommandQueue* commandQueue )
    {
#if ZP_DEBUG
        CallDebugUtil( vkCmdDebugMarkerEndEXT, m_vkInstance, static_cast<VkCommandBuffer>(commandQueue->commandBuffer) );
#endif
    }

    void VulkanGraphicsDevice::markEventLabel( const char* eventLabel, const Color& color, CommandQueue* commandQueue )
    {
#if ZP_DEBUG
        VkDebugMarkerMarkerInfoEXT debugMarkerMarkerInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,
            .pMarkerName = eventLabel,
            .color {
                color.r,
                color.g,
                color.b,
                color.a,
            }
        };

        CallDebugUtil( vkCmdDebugMarkerInsertEXT, m_vkInstance, static_cast<VkCommandBuffer>(commandQueue->commandBuffer), &debugMarkerMarkerInfo );
#endif
    }

//
//
//

    namespace
    {
        thread_local zp_bool_t t_isCommandPoolCreated;

        thread_local VkCommandPool t_vkGraphicsCommandPool;

        thread_local VkCommandPool t_vkTransferCommandPool;

        thread_local VkCommandPool t_vkComputeCommandPool;
    }

    VkCommandPool VulkanGraphicsDevice::getCommandPool( CommandQueue* commandQueue )
    {
        if( !t_isCommandPoolCreated )
        {
            t_isCommandPoolCreated = true;

            VkCommandPoolCreateInfo commandPoolCreateInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            };

            commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.graphicsQueue;
            HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, &m_vkAllocationCallbacks, &t_vkGraphicsCommandPool ) );

            commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.transferQueue;
            HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, &m_vkAllocationCallbacks, &t_vkTransferCommandPool ) );

            commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.computeQueue;
            HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, &m_vkAllocationCallbacks, &t_vkComputeCommandPool ) );

            const zp_size_t index = Atomic::AddSizeT( &m_commandPoolCount, 3 ) - 3;
            m_vkCommandPools[ index + 0 ] = t_vkGraphicsCommandPool;
            m_vkCommandPools[ index + 1 ] = t_vkTransferCommandPool;
            m_vkCommandPools[ index + 2 ] = t_vkComputeCommandPool;

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_COMMAND_POOL, t_vkGraphicsCommandPool, "Graphics Command Buffer Pool (%d)", Platform::GetCurrentThreadId() );
            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_COMMAND_POOL, t_vkTransferCommandPool, "Transfer Command Buffer Pool (%d)", Platform::GetCurrentThreadId() );
            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_COMMAND_POOL, t_vkComputeCommandPool, "Compute Command Buffer Pool (%d)", Platform::GetCurrentThreadId() );
        }

        const VkCommandPool commandPoolMap[] {
            t_vkGraphicsCommandPool,
            t_vkTransferCommandPool,
            t_vkComputeCommandPool,
            t_vkGraphicsCommandPool
        };

        return commandPoolMap[ commandQueue->queue ];
    }

    VkDescriptorSetLayout VulkanGraphicsDevice::getDescriptorSetLayout( const VkDescriptorSetLayoutCreateInfo& createInfo )
    {
        const zp_hash128_t hash = CalculateHash( createInfo );

        VkDescriptorSetLayout layout;
        if( !m_descriptorSetLayoutCache.tryGet( hash, &layout ) )
        {
            HR( vkCreateDescriptorSetLayout( m_vkLocalDevice, &createInfo, &m_vkAllocationCallbacks, &layout ) );

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, layout, "Descriptor Set Layout #%d (%d)", m_descriptorSetLayoutCache.size(), Platform::GetCurrentThreadId() );

            m_descriptorSetLayoutCache.set( hash, layout );
        }

        return layout;
    }

    void VulkanGraphicsDevice::processDelayedDestroy()
    {
        ZP_PROFILE_CPU_BLOCK();

        const zp_size_t kMaxFrameDistance = 3;
        if( !m_delayedDestroy.isEmpty() )
        {
            Vector<DelayedDestroy> handlesToDestroy( m_delayedDestroy.size(), MemoryLabels::Temp );

            // get delayed destroy handles that are too old
            for( zp_size_t i = 0; i < m_delayedDestroy.size(); ++i )
            {
                const DelayedDestroy& delayedDestroy = m_delayedDestroy[ i ];
                if( delayedDestroy.handle == nullptr )
                {
                    m_delayedDestroy.eraseAtSwapBack( i );
                    --i;
                }
                else if( m_currentFrameIndex < delayedDestroy.frameIndex || ( m_currentFrameIndex - delayedDestroy.frameIndex ) >= kMaxFrameDistance )
                {
                    handlesToDestroy.pushBack( delayedDestroy );

                    m_delayedDestroy.eraseAtSwapBack( i );
                    --i;
                }
            }

            if( !handlesToDestroy.isEmpty() )
            {
                // sort delayed destroy handles by how they were allocated
                handlesToDestroy.sort( []( const DelayedDestroy& a, const DelayedDestroy& b )
                {
                    return zp_cmp( a.order, b.order );
                } );

                // destroy each delayed destroy handle
                for( const DelayedDestroy& delayedDestroy : handlesToDestroy )
                {
                    DestroyDelayedDestroyHandle( delayedDestroy );
                }
            }
        }
    }

    void VulkanGraphicsDevice::destroyAllDelayedDestroy()
    {
        // destroy each delayed destroy handle
        for( const DelayedDestroy& delayedDestroy : m_delayedDestroy )
        {
            DestroyDelayedDestroyHandle( delayedDestroy );
        }

        m_delayedDestroy.clear();
    }

    void VulkanGraphicsDevice::rebuildSwapchain()
    {
        // cache swapchain values
        const zp_handle_t windowHandle = m_swapchainData.windowHandle;
        const zp_uint32_t width = 1; //zp_max( 1u, m_swapchainData.vkRequestedSwapChainExtent.width );
        const zp_uint32_t height = 1; //zp_max( 1u, m_swapchainData.vkRequestedSwapChainExtent.height );
        const int displayFormat = m_swapchainData.vkSwapChainFormat;
        const ColorSpace colorSpace = ZP_COLOR_SPACE_REC_709_LINEAR;
        const VkSwapchainKHR oldSwapchain = m_swapchainData.vkSwapchain;

        // TODO: handle minimize properly
        // recreate new swapchain using old values
        createSwapchain( windowHandle, width, height, displayFormat, colorSpace );
    }

    VulkanGraphicsDevice::PerFrameData& VulkanGraphicsDevice::getCurrentFrameData()
    {
        const zp_uint64_t frame = m_currentFrameIndex & ( kBufferedFrameCount - 1 );
        return m_perFrameData[ frame ];
    }

    const VulkanGraphicsDevice::PerFrameData& VulkanGraphicsDevice::getCurrentFrameData() const
    {
        const zp_uint64_t frame = m_currentFrameIndex & ( kBufferedFrameCount - 1 );
        return m_perFrameData[ frame ];
    }

    VulkanGraphicsDevice::PerFrameData& VulkanGraphicsDevice::getFrameData( zp_uint64_t frameIndex )
    {
        const zp_uint64_t frame = frameIndex & ( kBufferedFrameCount - 1 );
        return m_perFrameData[ frame ];
    }
#endif
}
