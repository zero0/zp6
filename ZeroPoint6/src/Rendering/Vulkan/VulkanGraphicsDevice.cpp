//
// Created by phosg on 2/3/2022.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Vector.h"
#include "Core/Set.h"
#include "Core/Math.h"
#include "Core/Atomic.h"
#include "Core/Profiler.h"
#include "Core/String.h"

#include "Platform/Platform.h"

#include "Engine/MemoryLabels.h"

#include "Rendering/GraphicsDevice.h"
#include "Rendering/Vulkan/VulkanGraphicsDevice.h"

#include <vulkan/vulkan.h>

#if ZP_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan_win32.h>

#endif

#if ZP_DEBUG
#define HR( r )   do { const VkResult ZP_CONCAT(_vkResult_,__LINE__) = (r); if( ZP_CONCAT(_vkResult_,__LINE__) != VK_SUCCESS ) { zp_error_printfln("HR Failed: " #r ); Platform::DebugBreak(); } } while( false )
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

        zp_hash128_t CalculateHash( const VkDescriptorSetLayoutCreateInfo& createInfo )
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

        zp_hash128_t CalculateHash( const VkRenderPassCreateInfo& createInfo )
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

            if( messageSeverity & messageMask )
            {
                if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
                {
                    zp_printfln( "[INFO] %s", pCallbackData->pMessage );
                }
                else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
                {
                    zp_printfln( "[WARN] %s", pCallbackData->pMessage );
                }
                else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
                {
                    zp_error_printfln( pCallbackData->pMessage );
                }

                if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
                {
                    MessageBoxResult result = Platform::ShowMessageBox( nullptr, "Vulkan Error", pCallbackData->pMessage, ZP_MESSAGE_BOX_TYPE_ERROR, ZP_MESSAGE_BOX_BUTTON_ABORT_RETRY_IGNORE );
                    if( result == ZP_MESSAGE_BOX_RESULT_ABORT )
                    {
                        exit( 1 );
                    }
                    else if( result == ZP_MESSAGE_BOX_RESULT_RETRY )
                    {
                        Platform::DebugBreak();
                    }
                }
            }

            const VkBool32 shouldAbort = messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? VK_TRUE : VK_FALSE;
            return shouldAbort;
        }

#endif // ZP_DEBUG

#if ZP_DEBUG

        void SetDebugObjectName( VkInstance vkInstance, VkDevice vkDevice, const char* name, VkObjectType objectType, void* objectHandle )
        {
            if( name )
            {
                VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfoExt {
                    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                    .objectType = objectType,
                    .objectHandle = reinterpret_cast<uint64_t>(objectHandle),
                    .pObjectName = name,
                };

                HR( CallDebugUtilResult( vkSetDebugUtilsObjectNameEXT, vkInstance, vkDevice, &debugUtilsObjectNameInfoExt ) );
            }
        }

        void SetDebugObjectName( VkInstance vkInstance, VkDevice vkDevice, VkObjectType objectType, void* objectHandle, const char* format, ... )
        {
            char name[512];

            va_list args;
            va_start( args, format );
            zp_snprintf( name, format, args );
            va_end( args );

            SetDebugObjectName( vkInstance, vkDevice, name, objectType, objectHandle );
        }

#else // !ZP_DEBUG
#define SetDebugObjectName(...) (void)0
#endif // ZP_DEBUG

#pragma endregion

        void PrintPhysicalDeviceInfo( VkPhysicalDevice physicalDevice )
        {
            VkPhysicalDeviceProperties properties {};
            vkGetPhysicalDeviceProperties( physicalDevice, &properties );

            MutableFixedString512 info;
            info.append( properties.deviceName );
            info.append( ' ' );

            SizeInfo sizeInfo = GetSizeInfoFromBytes( properties.limits.maxMemoryAllocationCount MB );
            info.appendFormat( "%1.1f %c%c", sizeInfo.size, sizeInfo.k, sizeInfo.b );
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

            if( properties.vendorID == 4318 ) // NVidia
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

            info.appendFormat( "%d.%d.%d.%d",
                VK_API_VERSION_VARIANT( properties.apiVersion ),
                VK_API_VERSION_MAJOR( properties.apiVersion ),
                VK_API_VERSION_MINOR( properties.apiVersion ),
                VK_API_VERSION_PATCH( properties.apiVersion ) );

            zp_printfln( info.c_str() );
        }

        zp_bool_t IsPhysicalDeviceSuitable( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const GraphicsDeviceDesc& graphicsDeviceDesc )
        {
            zp_bool_t isSuitable = true;

            VkPhysicalDeviceProperties physicalDeviceProperties;
            VkPhysicalDeviceFeatures physicalDeviceFeatures;
            vkGetPhysicalDeviceProperties( physicalDevice, &physicalDeviceProperties );
            vkGetPhysicalDeviceFeatures( physicalDevice, &physicalDeviceFeatures );

#if ZP_DEBUG
            zp_printfln( "Testing %s", physicalDeviceProperties.deviceName );
#endif
            // require discrete gpu
            isSuitable &= physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

            // support geometry shaders
            if( graphicsDeviceDesc.geometryShaderSupport )
            {
                isSuitable &= physicalDeviceFeatures.geometryShader;
            }

            // support tessellation shaders
            if( graphicsDeviceDesc.tessellationShaderSupport )
            {
                isSuitable &= physicalDeviceFeatures.tessellationShader;
            }

#if ZP_USE_PROFILER
            isSuitable &= physicalDeviceProperties.limits.timestampComputeAndGraphics;

            isSuitable &= physicalDeviceFeatures.pipelineStatisticsQuery;
#endif

            const char* kRequiredExtensions[] {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };

            uint32_t extensionCount = 0;
            vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &extensionCount, VK_NULL_HANDLE );

            Vector<VkExtensionProperties> availableExtensions( extensionCount, MemoryLabels::Temp );
            availableExtensions.resize_unsafe( extensionCount );

            vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &extensionCount, availableExtensions.data() );

            zp_uint32_t foundRequiredExtensions = 0;
            for( const VkExtensionProperties& availableExtension : availableExtensions )
            {
                zp_bool_t found = false;
                for( const char* requiredExtension : kRequiredExtensions )
                {
                    if( zp_strcmp( availableExtension.extensionName, requiredExtension ) == 0 )
                    {
                        found = true;
                        ++foundRequiredExtensions;
                        break;
                    }
                }
#if ZP_DEBUG
                zp_printfln( "[%c] %s", found ? 'X' : ' ', availableExtension.extensionName );
#endif
            }

            // check that required extensions are available
            const zp_bool_t areExtensionsAvailable = foundRequiredExtensions == ZP_ARRAY_SIZE( kRequiredExtensions );
            isSuitable &= areExtensionsAvailable;

            // check that surface has formats and present modes
            if( surface != VK_NULL_HANDLE && areExtensionsAvailable )
            {
                uint32_t formatCount = 0;
                vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, surface, &formatCount, VK_NULL_HANDLE );

                uint32_t presentModeCount = 0;
                vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, surface, &presentModeCount, VK_NULL_HANDLE );

                isSuitable &= formatCount != 0 && presentModeCount != 0;
            }

            return isSuitable;
        }

#pragma region Swapchain

        VkSurfaceFormatKHR ChooseSwapChainSurfaceFormat( const Vector<VkSurfaceFormatKHR>& surfaceFormats, int format, ColorSpace colorSpace )
        {
            VkSurfaceFormatKHR chosenFormat = surfaceFormats[ 0 ];

            const VkColorSpaceKHR searchColorSpace = Convert( colorSpace );

            for( const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats )
            {
                // VK_FORMAT_B8G8R8A8_SNORM
                if( surfaceFormat.format == format && surfaceFormat.colorSpace == searchColorSpace )
                {
                    chosenFormat = surfaceFormat;
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

        constexpr VkExtent2D ChooseSwapChainExtent( const VkSurfaceCapabilitiesKHR& surfaceCapabilities, uint32_t requestedWidth, uint32_t requestedHeight )
        {
            VkExtent2D chosenExtents;

            if( surfaceCapabilities.currentExtent.width == UINT32_MAX && surfaceCapabilities.currentExtent.height == UINT32_MAX )
            {
                chosenExtents.width = requestedWidth;
                chosenExtents.height = requestedHeight;
            }
            else
            {
                chosenExtents.width = zp_clamp(
                    requestedWidth,
                    surfaceCapabilities.minImageExtent.width,
                    surfaceCapabilities.maxImageExtent.width );
                chosenExtents.height = zp_clamp(
                    requestedHeight,
                    surfaceCapabilities.minImageExtent.height,
                    surfaceCapabilities.maxImageExtent.height );
            }

            return chosenExtents;
        }

#pragma endregion

        constexpr uint32_t FindMemoryTypeIndex( const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties, const zp_uint32_t typeFilter, const VkMemoryPropertyFlags memoryPropertyFlags )
        {
            for( uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++ )
            {
                const VkMemoryPropertyFlags flags = physicalDeviceMemoryProperties.memoryTypes[ i ].propertyFlags;
                if( ( typeFilter & ( 1 << i ) ) && ( flags & memoryPropertyFlags ) == memoryPropertyFlags )
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
            IMemoryAllocator* allocator = static_cast<IMemoryAllocator*>(pUserData);
            return allocator->allocate( size, alignment );
        }

        void* ReallocationCallback( void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope )
        {
            IMemoryAllocator* allocator = static_cast<IMemoryAllocator*>(pUserData);
            return allocator->reallocate( pOriginal, size, alignment );
        }

        void FreeCallback( void* pUserData, void* pMemory )
        {
            IMemoryAllocator* allocator = static_cast<IMemoryAllocator*>(pUserData);
            allocator->free( pMemory );
        }

#pragma endregion

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
    }

    //
    //
    //

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

        VkApplicationInfo applicationInfo {
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

        instanceCreateInfo.pNext = &createInstanceDebugMessengerInfo;
#endif
        HR( vkCreateInstance( &instanceCreateInfo, &m_vkAllocationCallbacks, &m_vkInstance ) );

#if ZP_DEBUG
        // create debug messenger
        VkDebugUtilsMessengerCreateInfoEXT createDebugMessengerInfo {
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
            VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo {
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

                if( m_queueFamilies.graphicsFamily == VK_QUEUE_FAMILY_IGNORED &&
                    queueFamilyProperty.queueCount > 0 &&
                    queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT )
                {
                    m_queueFamilies.graphicsFamily = i;

                    VkBool32 presentSupport = VK_FALSE;
                    HR( vkGetPhysicalDeviceSurfaceSupportKHR( m_vkPhysicalDevice, i, m_vkSurface, &presentSupport ) );

                    if( m_queueFamilies.presentFamily == VK_QUEUE_FAMILY_IGNORED && presentSupport )
                    {
                        m_queueFamilies.presentFamily = i;
                    }
                }

                if( m_queueFamilies.transferFamily == VK_QUEUE_FAMILY_IGNORED &&
                    queueFamilyProperty.queueCount > 0 &&
                    queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT )
                {
                    m_queueFamilies.transferFamily = i;
                }

                if( m_queueFamilies.computeFamily == VK_QUEUE_FAMILY_IGNORED &&
                    queueFamilyProperty.queueCount > 0 &&
                    queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT )
                {
                    m_queueFamilies.computeFamily = i;
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
                    m_queueFamilies.transferFamily = i;
                }

                if( queueFamilyProperty.queueCount > 0 &&
                    queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT &&
                    !( queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT ) )
                {
                    m_queueFamilies.computeFamily = i;
                }
            }

            Set<zp_uint32_t, zp_hash64_t, CastEqualityComparer<zp_uint32_t, zp_hash64_t>> uniqueFamilyIndices( 4, MemoryLabels::Temp );
            uniqueFamilyIndices.add( m_queueFamilies.graphicsFamily );
            uniqueFamilyIndices.add( m_queueFamilies.transferFamily );
            uniqueFamilyIndices.add( m_queueFamilies.computeFamily );
            uniqueFamilyIndices.add( m_queueFamilies.presentFamily );

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

            VkPhysicalDeviceSynchronization2Features synchronization2Features {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
                .pNext = nullptr,
                .synchronization2 = VK_TRUE
            };

            VkDeviceCreateInfo localDeviceCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &synchronization2Features,
                .queueCreateInfoCount = static_cast<uint32_t >(deviceQueueCreateInfos.size()),
                .pQueueCreateInfos = deviceQueueCreateInfos.data(),
                .enabledLayerCount = ZP_ARRAY_SIZE( kValidationLayers ),
                .ppEnabledLayerNames = kValidationLayers,
                .enabledExtensionCount = ZP_ARRAY_SIZE( kDeviceExtensions ),
                .ppEnabledExtensionNames = kDeviceExtensions,
                .pEnabledFeatures = &deviceFeatures,
            };

            HR( vkCreateDevice( m_vkPhysicalDevice, &localDeviceCreateInfo, &m_vkAllocationCallbacks, &m_vkLocalDevice ) );

            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.graphicsFamily, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_GRAPHICS ] );
            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.transferFamily, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_TRANSFER ] );
            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.computeFamily, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_COMPUTE ] );
            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.presentFamily, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_PRESENT ] );
        }

        // create pipeline cache
        {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo {
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
            //    commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.graphicsFamily;
            //    HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, nullptr, &m_vkGraphicsCommandPool ) );
            //
            //    commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.transferFamily;
            //    HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, nullptr, &m_vkTransferCommandPool ) );
            //
            //    commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.computeFamily;
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

            VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
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

        m_swapchainData.vkSwapchainExtent = ChooseSwapChainExtent( swapChainSupportDetails, width, height );
        if( m_swapchainData.vkSwapchainExtent.width == 0 && m_swapchainData.vkSwapchainExtent.height == 0 )
        {
            return;
        }

        uint32_t formatCount = 0;
        HR( vkGetPhysicalDeviceSurfaceFormatsKHR( m_vkPhysicalDevice, m_vkSurface, &formatCount, VK_NULL_HANDLE ) );

        Vector<VkSurfaceFormatKHR> surfaceFormats( formatCount, MemoryLabels::Temp );
        surfaceFormats.resize_unsafe( formatCount );

        HR( vkGetPhysicalDeviceSurfaceFormatsKHR( m_vkPhysicalDevice, m_vkSurface, &formatCount, surfaceFormats.data() ) );

        uint32_t presentModeCount = 0;
        HR( vkGetPhysicalDeviceSurfacePresentModesKHR( m_vkPhysicalDevice, m_vkSurface, &presentModeCount, VK_NULL_HANDLE ) );

        Vector<VkPresentModeKHR> presentModes( presentModeCount, MemoryLabels::Temp );
        presentModes.resize_unsafe( presentModeCount );

        HR( vkGetPhysicalDeviceSurfacePresentModesKHR( m_vkPhysicalDevice, m_vkSurface, &presentModeCount, presentModes.data() ) );

        // TODO: remove when enums are created
        displayFormat = VK_FORMAT_B8G8R8A8_SNORM;

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapChainSurfaceFormat( surfaceFormats, displayFormat, colorSpace );
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
            .imageExtent = m_swapchainData.vkSwapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .preTransform = swapChainSupportDetails.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = m_swapchainData.vkSwapchain
        };

        const uint32_t queueFamilyIndices[] = { m_queueFamilies.graphicsFamily, m_queueFamilies.presentFamily };

        if( m_queueFamilies.graphicsFamily != m_queueFamilies.presentFamily )
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
                .width = m_swapchainData.vkSwapchainExtent.width,
                .height = m_swapchainData.vkSwapchainExtent.height,
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

        VkResult result;

        result = vkAcquireNextImageKHR( m_vkLocalDevice, m_swapchainData.vkSwapchain, UINT64_MAX, currentFrameData.vkSwapChainAcquireSemaphore, VK_NULL_HANDLE, &currentFrameData.swapChainImageIndex );

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

        PerFrameData& currentFrameData = getCurrentFrameData();

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

        VkPipelineStageFlags waitStages[] { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore waitSemaphores[] { currentFrameData.vkSwapChainAcquireSemaphore };
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

        PerFrameData& currentFrameData = getCurrentFrameData();

        VkSemaphore waitSemaphores[] { currentFrameData.vkRenderFinishedSemaphore };

        VkSwapchainKHR swapchains[] { m_swapchainData.vkSwapchain };
        uint32_t imageIndices[] { currentFrameData.swapChainImageIndex };
        ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( swapchains ) == ZP_ARRAY_SIZE( imageIndices ) );

        const VkPresentInfoKHR presentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = ZP_ARRAY_SIZE( waitSemaphores ),
            .pWaitSemaphores = waitSemaphores,
            .swapchainCount = ZP_ARRAY_SIZE( swapchains ),
            .pSwapchains = swapchains,
            .pImageIndices = imageIndices,
        };

        VkResult result;
        result = vkQueuePresentKHR( m_vkRenderQueues[ ZP_RENDER_QUEUE_PRESENT ], &presentInfo );
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
            .pDynamicState = nullptr,
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
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};

        VkSamplerCreateInfo samplerCreateInfo {};

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
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
                .extent { m_swapchainData.vkSwapchainExtent },
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

            commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.graphicsFamily;
            HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, &m_vkAllocationCallbacks, &t_vkGraphicsCommandPool ) );

            commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.transferFamily;
            HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, &m_vkAllocationCallbacks, &t_vkTransferCommandPool ) );

            commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.computeFamily;
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

    VulkanGraphicsDevice::PerFrameData& VulkanGraphicsDevice::getFrameData( zp_uint64_t frameIndex )
    {
        const zp_uint64_t frame = frameIndex & ( kBufferedFrameCount - 1 );
        return m_perFrameData[ frame ];
    }
}
