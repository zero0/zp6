//
// Created by phosg on 2/3/2022.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Memory.h"
#include "Core/Allocator.h"
#include "Core/Vector.h"
#include "Core/Set.h"
#include "Core/Math.h"
#include "Core/Profiler.h"
#include "Core/String.h"
#include "Core/Log.h"
#include "Core/Job.h"
#include "Core/Data.h"
#include "Core/Hash.h"
#include "Core/Queue.h"

#include "Platform/Platform.h"

#include "Engine/MemoryLabels.h"

#include "Rendering/GraphicsDevice.h"
#include "Rendering/GraphicsTypes.h"
#include "Rendering/Vulkan/VulkanGraphicsDevice.h"
#include "Volk/volk.h"

// ZP_CONCAT(_vkResult_,__LINE__)
#if ZP_DEBUG
#define HR( r )                                             \
do                                                          \
{                                                           \
    if( const VkResult result = (r); result != VK_SUCCESS ) \
    {                                                       \
        Log::error() << #r << Log::endl;                    \
        Platform::DebugBreak();                             \
    }                                                       \
}                                                           \
while( false )
#else
#define HR( r )   r
#endif

#define FlagBits( f, z, t, v )         f |= ( (z) & (t) ) ? (v) : 0
#define USE_DYNAMIC_RENDERING   1

namespace zp
{
    namespace
    {
        enum
        {
            kMaxBufferedFrameCount = 4,
            kMaxMipLevels = 16,
        };

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

        struct VulkanBuffer
        {
            VkBuffer vkBuffer;
            VkDeviceMemory vkDeviceMemory;
            VkBufferUsageFlags vkBufferUsage;
            VkAccessFlags2 vkAccess;

            zp_size_t offset;
            zp_size_t alignment;
            zp_size_t size;

            zp_hash32_t hash;
        };

        struct VulkanTexture
        {
            VkImage vkImage;
            FixedArray<VkImageView, kMaxMipLevels> vkImageViews;
            FixedArray<VkImageLayout, kMaxMipLevels> vkImageLayouts;
            VkDeviceMemory vkDeviceMemory;
            VkImageUsageFlags vkImageUsage;
            VkImageAspectFlags vkImageAspect;
            VkFormat vkFormat;

            Size3Du size;
            zp_uint32_t mipCount;
            zp_uint32_t loadedMipFlag;

            zp_hash32_t hash;
        };

        struct VulkanRenderTarget
        {
            VkImage vkImage;
            FixedArray<VkImageView, kMaxBufferedFrameCount> vkImageViews;
            FixedArray<VkImageLayout, kMaxBufferedFrameCount> vkImageLayouts;
            VkDeviceMemory vkDeviceMemory;
            VkImageUsageFlags vkImageUsage;
            VkImageAspectFlags vkImageAspect;
            VkFormat vkFormat;

            Size3Du size;
            zp_uint32_t sampleCount;

            zp_hash32_t hash;
        };

        struct VulkanSampler
        {
            VkSampler vkSampler;

            zp_hash32_t hash;
        };

        struct VulkanCommandQueue
        {
            VkCommandBuffer vkCommandBuffer;

            VkCommandBufferUsageFlags usage;
            zp_hash32_t hash;
        };

        struct StagingBuffer
        {
            VkBuffer vkBuffer;
            VkDeviceMemory vkDeviceMemory;

            zp_size_t position;
            zp_size_t alignment;
            zp_size_t size;
        };

        struct StagingBufferAllocation
        {
            VkBuffer vkBuffer;
            VkDeviceMemory vkDeviceMemory;
            VkAccessFlags2 vkAccess;

            zp_size_t offset;
            zp_size_t size;
        };

        struct FrameResources
        {
            StagingBuffer stagingBuffer;

            VkCommandPool vkCommandPool;
            VkDescriptorSet vkBindlessDescriptorSet;
        };


#pragma region GetObjectType

        template<typename T>
        constexpr auto GetObjectType( T /*unused*/ ) -> VkObjectType
        {
            ZP_ASSERT_MSG_ARGS( false, "Unknown Object Type: %s", zp_type_name<T>() );
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

        MAKE_OBJECT_TYPE( VkPipelineCache, VK_OBJECT_TYPE_PIPELINE_CACHE );

        MAKE_OBJECT_TYPE( VkDevice, VK_OBJECT_TYPE_DEVICE );

        MAKE_OBJECT_TYPE( VkQueue, VK_OBJECT_TYPE_QUEUE );

        MAKE_OBJECT_TYPE( VkSemaphore, VK_OBJECT_TYPE_SEMAPHORE );

        MAKE_OBJECT_TYPE( VkCommandPool, VK_OBJECT_TYPE_COMMAND_POOL );

        MAKE_OBJECT_TYPE( VkCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER );

        MAKE_OBJECT_TYPE( VkFence, VK_OBJECT_TYPE_FENCE );

        MAKE_OBJECT_TYPE( VkDeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY );

        MAKE_OBJECT_TYPE( VkBuffer, VK_OBJECT_TYPE_BUFFER );

        MAKE_OBJECT_TYPE( VkBufferView, VK_OBJECT_TYPE_BUFFER_VIEW );

        MAKE_OBJECT_TYPE( VkImage, VK_OBJECT_TYPE_IMAGE );

        MAKE_OBJECT_TYPE( VkImageView, VK_OBJECT_TYPE_IMAGE_VIEW );

        MAKE_OBJECT_TYPE( VkSwapchainKHR, VK_OBJECT_TYPE_SWAPCHAIN_KHR );

        MAKE_OBJECT_TYPE( VkDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL );

        MAKE_OBJECT_TYPE( VkDescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET );

        MAKE_OBJECT_TYPE( VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT );

#undef MAKE_OBJECT_TYPE

#pragma endregion

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
                if( zp_flag32_all_set( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ) )
                {
                    Log::info() << pCallbackData->pMessage << Log::endl;
                }
                else if( zp_flag32_all_set( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) )
                {
                    Log::warning() << pCallbackData->pMessage << Log::endl;
                }
                else if( zp_flag32_all_set( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) )
                {
                    Log::error() << pCallbackData->pMessage << Log::endl;
                }

                if( zp_flag32_all_set( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) )
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

            const VkBool32 shouldAbort = zp_flag32_all_set( messageSeverity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) ? VK_TRUE : VK_FALSE;
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

        template<typename T, typename ... Args>
        void SetDebugObjectName( VkInstance vkInstance, VkDevice vkDevice, T objectHandle, const char* format, Args... args )
        {
            MutableFixedString512 name;
            name.appendFormat( format, args... );

            SetDebugObjectName( vkInstance, vkDevice, GetObjectType( objectHandle ), objectHandle, name.c_str() );
        }

#else // !ZP_DEBUG
#define SetDebugObjectName(...) (void)0
#endif // ZP_DEBUG

#pragma endregion

#pragma region Physical Device Utils
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

#pragma endregion

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

#pragma endregion

#pragma region Pipeline Transition
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

                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    return {
                        .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                        .access = VK_ACCESS_2_TRANSFER_READ_BIT
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

        VkPipelineStageFlags2 MakePipelineStageAccess( VkAccessFlags2 access )
        {
            if( access == VK_ACCESS_2_HOST_WRITE_BIT )
            {
                return VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            }
            else
            {
                return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
        }

        void CmdTransitionBufferAccess( VkCommandBuffer cmd, VkBuffer buffer, zp_size_t offset, zp_size_t size, VkAccessFlags2& srcAccess, VkAccessFlags2 dstAccess )
        {
            const VkPipelineStageFlags2 srcStage = MakePipelineStageAccess( srcAccess );
            const VkPipelineStageFlags2 dstStage = MakePipelineStageAccess( dstAccess );

            const VkBufferMemoryBarrier2 memoryBarrier {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .srcStageMask = srcStage,
                .srcAccessMask = srcAccess,
                .dstStageMask = dstStage,
                .dstAccessMask = dstAccess,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = buffer,
                .offset = offset,
                .size = size,
            };

            const VkDependencyInfo dependencyInfo {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .bufferMemoryBarrierCount = 1,
                .pBufferMemoryBarriers = &memoryBarrier,
            };

            vkCmdPipelineBarrier2( cmd, &dependencyInfo );

            srcAccess = dstAccess;
        }

        void CmdTransitionBufferAccess( VkCommandBuffer cmd, VulkanBuffer& buffer, zp_size_t offset, zp_size_t size, VkAccessFlags2 dstAccess )
        {
            if( buffer.vkAccess != dstAccess )
            {
                const VkPipelineStageFlags2 srcStage = MakePipelineStageAccess( buffer.vkAccess );
                const VkPipelineStageFlags2 dstStage = MakePipelineStageAccess( dstAccess );

                const VkBufferMemoryBarrier2 memoryBarrier {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .srcStageMask = srcStage,
                    .srcAccessMask = buffer.vkAccess,
                    .dstStageMask = dstStage,
                    .dstAccessMask = dstAccess,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = buffer.vkBuffer,
                    .offset = buffer.offset + offset,
                    .size = size,
                };

                const VkDependencyInfo dependencyInfo {
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .bufferMemoryBarrierCount = 1,
                    .pBufferMemoryBarriers = &memoryBarrier,
                };

                vkCmdPipelineBarrier2( cmd, &dependencyInfo );

                buffer.vkAccess = dstAccess;
            }
        }

        void CmdTransitionBufferAccess( VkCommandBuffer cmd, VulkanBuffer& buffer, VkAccessFlags2 dstAccess )
        {
            CmdTransitionBufferAccess( cmd, buffer, 0, buffer.size, dstAccess );
        }

        void CmdTransitionImageLayout( VkCommandBuffer cmd, VkImage image, VkImageLayout& srcLayout, VkImageLayout dstLayout, const VkImageSubresourceRange& subresourceRange )
        {
            const auto [srcStage, srcAccess] = MakePipelineStageAccess( srcLayout );
            const auto [dstStage, dstAccess] = MakePipelineStageAccess( dstLayout );

            const VkImageMemoryBarrier2 imageMemoryBarrier {
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
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
                .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers    = &imageMemoryBarrier,
            };

            vkCmdPipelineBarrier2( cmd, &dependencyInfo );

            srcLayout = dstLayout;
        }

        void CmdTransitionImageLayout( VkCommandBuffer cmd, VulkanTexture& texture, VkImageLayout dstLayout, const VkImageSubresourceRange& subresourceRange )
        {
            const auto [dstStage, dstAccess] = MakePipelineStageAccess( dstLayout );

            const zp_uint32_t maxMip = subresourceRange.baseMipLevel + subresourceRange.levelCount;

            FixedVector<VkImageMemoryBarrier2, kMaxMipLevels> imageMemoryBarriers;

#if 1
            // brute force transition each loaded mip level
            for( zp_uint32_t mip = subresourceRange.baseMipLevel; mip < maxMip; ++mip )
            {
                const VkImageLayout currentLayout = texture.vkImageLayouts[ mip ];
                if( zp_flag32_is_bit_set( texture.loadedMipFlag, mip ) && currentLayout != dstLayout )
                {
                    const auto [srcStage, srcAccess] = MakePipelineStageAccess( currentLayout );

                    imageMemoryBarriers.pushBack( {
                        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                        .srcStageMask        = srcStage,
                        .srcAccessMask       = srcAccess,
                        .dstStageMask        = dstStage,
                        .dstAccessMask       = dstAccess,
                        .oldLayout           = currentLayout,
                        .newLayout           = dstLayout,
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .image               = texture.vkImage,
                        .subresourceRange {
                            .aspectMask = subresourceRange.aspectMask,
                            .baseMipLevel = mip,
                            .levelCount = maxMip - mip,
                            .baseArrayLayer = subresourceRange.baseArrayLayer,
                            .layerCount = subresourceRange.layerCount,
                        },
                    } );
                }
            }
#else
            VkImageLayout currentLayout = texture.vkImageLayouts[ 0 ];
            zp_uint32_t currentMip = subresourceRange.baseMipLevel;

            // group mip updates based on when the layout changes
            for( zp_uint32_t mip = subresourceRange.baseMipLevel; mip < maxMip; ++mip )
            {
                if( currentLayout != texture.vkImageLayouts[ mip ] )
                {
                    const auto [srcStage, srcAccess] = MakePipelineStageAccess( currentLayout );

                    imageMemoryBarriers.pushBack( {
                        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                        .srcStageMask        = srcStage,
                        .srcAccessMask       = srcAccess,
                        .dstStageMask        = dstStage,
                        .dstAccessMask       = dstAccess,
                        .oldLayout           = currentLayout,
                        .newLayout           = dstLayout,
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .image               = texture.vkImage,
                        .subresourceRange {
                            .aspectMask = subresourceRange.aspectMask,
                            .baseMipLevel = currentMip,
                            .levelCount = mip - currentMip,
                            .baseArrayLayer = subresourceRange.baseArrayLayer,
                            .layerCount = subresourceRange.layerCount,
                        },
                    } );

                    currentLayout = texture.vkImageLayouts[ mip ];
                    currentMip = mip;
                }
            }

            // if there are any remaining mips
            if( currentMip < maxMip )
            {
                const auto [srcStage, srcAccess] = MakePipelineStageAccess( currentLayout );

                imageMemoryBarriers.pushBack( {
                    .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .srcStageMask        = srcStage,
                    .srcAccessMask       = srcAccess,
                    .dstStageMask        = dstStage,
                    .dstAccessMask       = dstAccess,
                    .oldLayout           = currentLayout,
                    .newLayout           = dstLayout,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image               = texture.vkImage,
                    .subresourceRange {
                        .aspectMask = subresourceRange.aspectMask,
                        .baseMipLevel = currentMip,
                        .levelCount = maxMip - currentMip,
                        .baseArrayLayer = subresourceRange.baseArrayLayer,
                        .layerCount = subresourceRange.layerCount,
                    },
                } );
            }
#endif

            if( !imageMemoryBarriers.isEmpty() )
            {
                const VkDependencyInfo dependencyInfo {
                    .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .imageMemoryBarrierCount = static_cast<zp_uint32_t>( imageMemoryBarriers.length() ),
                    .pImageMemoryBarriers    = imageMemoryBarriers.data(),
                };

                vkCmdPipelineBarrier2( cmd, &dependencyInfo );
            }

            // update layouts
            for( zp_uint32_t mip = subresourceRange.baseMipLevel; mip < maxMip; ++mip )
            {
                texture.vkImageLayouts[ mip ] = dstLayout;
            }
        }

#pragma endregion

#pragma region Single Use CommandBuffer

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
                if( zp_flag32_is_bit_set( typeFilter, i ) && zp_flag32_all_set( flags, memoryPropertyFlags ) )
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

#pragma region Delayed Destroy
        struct DelayedDestroyInfo
        {
            zp_uint64_t frame;
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

#pragma endregion

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
    }

    namespace
    {
        class VulkanContext
        {
        public:
            VulkanContext();

            ~VulkanContext();

            void Initialize( const GraphicsDeviceDesc& graphicsDeviceDesc );

            void Destroy();

            void BeginFrame( zp_uint64_t frame, zp_size_t frameIndex );

            void EndFrame( VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence fence );

            void FlushDestroyQueue();

            StagingBufferAllocation Allocate( zp_size_t size );

#pragma region Buffer

            BufferHandle RequestBuffer( zp_size_t size );

            void ReleaseBuffer( BufferHandle bufferHandle );

            void UpdateBufferData( CommandQueueHandle commandQueueHandle, BufferHandle dstBuffer, zp_size_t dstOffset, Memory srcData );

#pragma endregion

#pragma region Texture

            TextureHandle RequestTexture();

            void ReleaseTexture( TextureHandle texture );

            void UpdateTextureMipData( CommandQueueHandle commandQueue, TextureHandle dstTexture, zp_uint32_t mipLevel, Memory srcData );

            void GenerateTextureMipLevels( CommandQueueHandle commandQueue, TextureHandle texture, zp_uint32_t mipLevelFlagsToGenerate );

#pragma endregion

#pragma region Shader

            ShaderHandle RequestShader();

            void ReleaseShader( ShaderHandle shader );

#pragma endregion

#pragma region RenderPass

            RenderPassHandle RequestRenderPass();

            void ReleaseRenderPass( RenderPassHandle rendrPass );

#pragma endregion

            void BeginRenderPass( const CommandBeginRenderPass& cmd, MemoryArray<CommandBeginRenderPass::ColorAttachment> colorAttachments );

            void NextSubpass( const CommandNextSubpass& cmd );

            void EndRenderPass( const CommandEndRenderPass& cmd );

            void Dispatch( const CommandDispatch& cmd );

            void DispatchIndirect( const CommandDispatchIndirect& cmd );

            void Draw( const CommandDraw& cmd );

            void Blit( const CommandBlit& cmd );

            void UseBuffer( BufferHandle bufferHandle );

            void UseTexture( TextureHandle textureHandle );

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
            void ForceFlushDestroyQueue();

            VkInstance m_vkInstance;
            VkPhysicalDevice m_vkPhysicalDevice;
            VkDevice m_vkLocalDevice;
            VkSurfaceKHR m_vkSurface;

            VkPipelineCache m_vkPipelineCache;
            VkDescriptorPool m_vkDescriptorPool;
            VkCommandPool m_vkTransientCommandPool;

            VkDescriptorSetLayout m_vkBindlessLayout;
            VkDescriptorPool m_vkBindlessDescriptorPool;

            FixedArray<FrameResources, kMaxBufferedFrameCount> m_frameResources;

            FixedArray<VkCommandPool, kMaxBufferedFrameCount> m_vkCommandPools;
            FixedArray<StagingBuffer, kMaxBufferedFrameCount> m_stagingBuffers;
            FixedArray<VkDescriptorSet, kMaxBufferedFrameCount> m_vkBindlessDescriptorSets;

#if ZP_DEBUG
            VkDebugUtilsMessengerEXT m_vkDebugMessenger;
#endif
            VkAllocationCallbacks m_vkAllocationCallbacks;

            VkPhysicalDeviceMemoryProperties m_vkPhysicalDeviceMemoryProperties;

            Queues m_queues;

            Vector<DelayedDestroyInfo> m_delayedDestroyed;

            Vector<VulkanCommandQueue> m_vkCommandQueues;
            Queue<zp_uint32_t> m_vkFreeCommandQueues;

            Vector<VulkanBuffer> m_vkBuffers;
            Queue<zp_uint32_t> m_vkFreeBuffers;

            Vector<VulkanTexture> m_vkTextures;
            Queue<zp_uint32_t> m_vkFreeTextures;

            Vector<VulkanRenderTarget> m_vkRenderTargets;
            Queue<zp_uint32_t> m_vkFreeRenderTargets;

            zp_uint64_t m_frame;
            zp_size_t m_frameIndex;
        };

        VulkanContext::VulkanContext()
            : m_delayedDestroyed( 8, MemoryLabels::Graphics )
            , m_vkCommandQueues( 4, MemoryLabels::Graphics )
            , m_vkFreeCommandQueues( 4, MemoryLabels::Graphics )
            , m_vkBuffers( 16, MemoryLabels::Graphics )
            , m_vkFreeBuffers( 16, MemoryLabels::Graphics )
        {
        }

        VulkanContext::~VulkanContext()
        {
            ZP_ASSERT( m_vkInstance == VK_NULL_HANDLE );
            ZP_ASSERT( m_vkBuffers.isEmpty() );
        }

        void VulkanContext::Initialize( const GraphicsDeviceDesc& graphicsDeviceDesc )
        {
            ZP_PROFILE_CPU_BLOCK();

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

            constexpr const char* kDeviceExtensions[] {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
                VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
                VK_KHR_MAINTENANCE_6_EXTENSION_NAME,
                VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
                VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
                VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
                VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
                VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME,
                VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
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

                vkGetPhysicalDeviceMemoryProperties( m_vkPhysicalDevice, &m_vkPhysicalDeviceMemoryProperties );
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
                        zp_flag32_any_set( queueFamilyProperty.queueFlags, VK_QUEUE_GRAPHICS_BIT ) )
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
                        zp_flag32_any_set( queueFamilyProperty.queueFlags, VK_QUEUE_TRANSFER_BIT ) )
                    {
                        m_queues.transfer.familyIndex = i;
                    }

                    if( m_queues.compute.familyIndex == VK_QUEUE_FAMILY_IGNORED &&
                        zp_flag32_any_set( queueFamilyProperty.queueFlags, VK_QUEUE_COMPUTE_BIT ) )
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

                    if( zp_flag32_any_set( queueFamilyProperty.queueFlags, VK_QUEUE_TRANSFER_BIT ) &&
                        !zp_flag32_any_set( queueFamilyProperty.queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) )
                    {
                        m_queues.transfer.familyIndex = i;
                    }

                    if( zp_flag32_any_set( queueFamilyProperty.queueFlags, VK_QUEUE_COMPUTE_BIT ) &&
                        !zp_flag32_any_set( queueFamilyProperty.queueFlags, VK_QUEUE_GRAPHICS_BIT ) )
                    {
                        m_queues.compute.familyIndex = i;
                    }
                }

                Set<zp_uint32_t, zp_hash64_t, CastEqualityComparer<zp_uint32_t, zp_hash64_t>> uniqueFamilyIndices( 4, MemoryLabels::Temp );
                uniqueFamilyIndices.add( m_queues.graphics.familyIndex );
                uniqueFamilyIndices.add( m_queues.transfer.familyIndex );
                uniqueFamilyIndices.add( m_queues.compute.familyIndex );
                uniqueFamilyIndices.add( m_queues.present.familyIndex );

                FixedVector<VkDeviceQueueCreateInfo, 4> deviceQueueCreateInfos;

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
                    .queueCreateInfoCount = static_cast<uint32_t>( deviceQueueCreateInfos.length() ),
                    .pQueueCreateInfos = deviceQueueCreateInfos.data(),
                    .enabledLayerCount = ZP_ARRAY_SIZE( kValidationLayers ),
                    .ppEnabledLayerNames = kValidationLayers,
                    .enabledExtensionCount = static_cast<zp_uint32_t>( supportedExtensions.length() ),
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

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_queues.graphics.vkQueue, "Graphics Queue" );
                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_queues.transfer.vkQueue, "Transfer Queue" );
                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_queues.compute.vkQueue, "Compute Queue" );
                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_queues.present.vkQueue, "Present Queue" );
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

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_vkPipelineCache, "Pipeline Cache" );
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

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_vkDescriptorPool, "Descriptor Pool" );
            }

            // create transient command pool
            {
                const VkCommandPoolCreateInfo transientCommandPoolCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                    .queueFamilyIndex = m_queues.graphics.familyIndex,
                };

                HR( vkCreateCommandPool( m_vkLocalDevice, &transientCommandPoolCreateInfo, &m_vkAllocationCallbacks, &m_vkTransientCommandPool ) );

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_vkTransientCommandPool, "Transient Command Pool" );
            }

            // create per frame command pools
            {
                const VkCommandPoolCreateInfo poolCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                    .queueFamilyIndex = m_queues.graphics.familyIndex,
                };

                for( zp_size_t i = 0; i < kMaxBufferedFrameCount; ++i )
                {
                    HR( vkCreateCommandPool( m_vkLocalDevice, &poolCreateInfo, &m_vkAllocationCallbacks, &m_vkCommandPools[ i ] ) );

                    SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_vkCommandPools[ i ], "Command Pool %d", i );
                }
            }

            // create staging buffers
            {
                const VkBufferCreateInfo stagingBufferCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    .flags = 0,
                    .size = graphicsDeviceDesc.stagingBufferSize,
                    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndexCount = 0,
                    .pQueueFamilyIndices = nullptr,
                };

                for( zp_size_t i = 0; i < kMaxBufferedFrameCount; ++i )
                {
                    VkBuffer vkBuffer {};
                    HR( vkCreateBuffer( m_vkLocalDevice, &stagingBufferCreateInfo, &m_vkAllocationCallbacks, &vkBuffer ) );

                    VkMemoryRequirements memoryRequirements {};
                    vkGetBufferMemoryRequirements( m_vkLocalDevice, vkBuffer, &memoryRequirements );

                    const VkMemoryAllocateInfo memoryAllocateInfo {
                        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                        .allocationSize = memoryRequirements.size,
                        .memoryTypeIndex = FindMemoryTypeIndex( m_vkPhysicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ),
                    };

                    // TODO: allocate single block of memory for all staging buffers to use?
                    VkDeviceMemory vkDeviceMemory {};
                    HR( vkAllocateMemory( m_vkLocalDevice, &memoryAllocateInfo, &m_vkAllocationCallbacks, &vkDeviceMemory ) );

                    HR( vkBindBufferMemory( m_vkLocalDevice, vkBuffer, vkDeviceMemory, 0 ) );

                    SetDebugObjectName( m_vkInstance, m_vkLocalDevice, vkBuffer, "Staging Buffer %d", i );

                    m_stagingBuffers[ i ] = StagingBuffer {
                        .vkBuffer = vkBuffer,
                        .vkDeviceMemory = vkDeviceMemory,
                        .position = 0,
                        .alignment = memoryRequirements.alignment,
                        .size = memoryAllocateInfo.allocationSize,
                    };
                };
            }

            // create bindless setup
            {
                const FixedArray<VkDescriptorSetLayoutBinding, 3> bindings {
                    VkDescriptorSetLayoutBinding {
                        .binding = 0,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .descriptorCount = 1024,
                        .stageFlags = VK_SHADER_STAGE_ALL,
                    },
                    VkDescriptorSetLayoutBinding {
                        .binding = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                        .descriptorCount = 1024,
                        .stageFlags = VK_SHADER_STAGE_ALL,
                    },
                    VkDescriptorSetLayoutBinding {
                        .binding = 2,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = 1024,
                        .stageFlags = VK_SHADER_STAGE_ALL,
                    },
                };

                const FixedArray<VkDescriptorBindingFlags, 3> flags {
                    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                    VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                };
                ZP_ASSERT( bindings.length() == flags.length() );

                const VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
                    .bindingCount = static_cast<uint32_t>( flags.length() ),
                    .pBindingFlags = flags.data(),
                };

                const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                    .pNext = &bindingFlagsCreateInfo,
                    .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
                    .bindingCount = static_cast<uint32_t>( bindings.length() ),
                    .pBindings = bindings.data(),
                };

                HR( vkCreateDescriptorSetLayout( m_vkLocalDevice, &descriptorSetLayoutCreateInfo, &m_vkAllocationCallbacks, &m_vkBindlessLayout ) );

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_vkBindlessLayout, "Bindless Layout" );

                const VkDescriptorPoolCreateInfo poolInfo {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                    .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                    .maxSets = 16,
                };

                HR( vkCreateDescriptorPool( m_vkLocalDevice, &poolInfo, &m_vkAllocationCallbacks, &m_vkBindlessDescriptorPool ) );

                SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_vkBindlessDescriptorPool, "Bindless Descriptor Pool" );

                const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                    .descriptorPool = m_vkBindlessDescriptorPool,
                    .descriptorSetCount = 1,
                    .pSetLayouts = &m_vkBindlessLayout,
                };

                for( zp_size_t i = 0; i < kMaxBufferedFrameCount; ++i )
                {
                    HR( vkAllocateDescriptorSets( m_vkLocalDevice, &descriptorSetAllocateInfo, &m_vkBindlessDescriptorSets[ i ] ) );

                    SetDebugObjectName( m_vkInstance, m_vkLocalDevice, m_vkBindlessDescriptorSets[ i ], "Bindless Descriptor Set %d", i );
                }
            };
        }

        void VulkanContext::Destroy()
        {
            ZP_PROFILE_CPU_BLOCK();

            ZP_ASSERT( m_vkLocalDevice );

            HR( vkDeviceWaitIdle( m_vkLocalDevice ) );

            ForceFlushDestroyQueue();

            vkDestroySurfaceKHR( m_vkInstance, m_vkSurface, &m_vkAllocationCallbacks );

            // TODO: write out pipeline cache?
            vkDestroyPipelineCache( m_vkLocalDevice, m_vkPipelineCache, &m_vkAllocationCallbacks );

            vkDestroyDescriptorPool( m_vkLocalDevice, m_vkDescriptorPool, &m_vkAllocationCallbacks );

            vkDestroyDescriptorPool( m_vkLocalDevice, m_vkBindlessDescriptorPool, &m_vkAllocationCallbacks );

            vkDestroyDescriptorSetLayout( m_vkLocalDevice, m_vkBindlessLayout, &m_vkAllocationCallbacks );

            // destroy command pools
            vkDestroyCommandPool( m_vkLocalDevice, m_vkTransientCommandPool, &m_vkAllocationCallbacks );

            for( VkCommandPool pool : m_vkCommandPools )
            {
                vkDestroyCommandPool( m_vkLocalDevice, pool, &m_vkAllocationCallbacks );
            }

            // destroy staging buffer
            for( auto& stagingBuffer : m_stagingBuffers )
            {
                vkDestroyBuffer( m_vkLocalDevice, stagingBuffer.vkBuffer, &m_vkAllocationCallbacks );

                vkFreeMemory( m_vkLocalDevice, stagingBuffer.vkDeviceMemory, &m_vkAllocationCallbacks );
            }

#if ZP_DEBUG
            CallDebugUtil( vkDestroyDebugUtilsMessengerEXT, m_vkInstance, m_vkInstance, m_vkDebugMessenger, &m_vkAllocationCallbacks );
#endif // ZP_DEBUG

            vkDestroyDevice( m_vkLocalDevice, &m_vkAllocationCallbacks );
            vkDestroyInstance( m_vkInstance, &m_vkAllocationCallbacks );

            volkFinalize();

            m_vkInstance = nullptr;
            m_vkLocalDevice = nullptr;

#if ZP_DEBUG
            m_vkAllocationCallbacks = {};
#endif // ZP_DEBUG

            m_vkDescriptorPool = nullptr;
            m_vkPipelineCache = nullptr;
        }

        void VulkanContext::BeginFrame( zp_uint64_t frame, zp_size_t frameIndex )
        {
            ZP_PROFILE_CPU_BLOCK();

            m_frame = frame;
            m_frameIndex = frameIndex;

            m_stagingBuffers[ frameIndex ].position = 0;

            FlushDestroyQueue();
        }

        void VulkanContext::EndFrame( VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkFence fence )
        {
            ZP_PROFILE_CPU_BLOCK();

            const VkSemaphoreSubmitInfo waitSemaphores {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = waitSemaphore,
                .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            };
            const VkCommandBufferSubmitInfo commandBuffers {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = nullptr,
            };
            const VkSemaphoreSubmitInfo signalSemaphores {
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

        void VulkanContext::FlushDestroyQueue()
        {
            ZP_PROFILE_CPU_BLOCK();

            const zp_size_t kMaxFrameDistance = 3;
            if( !m_delayedDestroyed.isEmpty() )
            {
                Vector<DelayedDestroyInfo> handlesToDestroy( m_delayedDestroyed.length(), MemoryLabels::Temp );

                // get delayed destroy handles that are too old
                for( zp_size_t i = 0; i < m_delayedDestroyed.length(); ++i )
                {
                    const auto& destroyInfo = m_delayedDestroyed[ i ];
                    if( destroyInfo.vkHandle == nullptr )
                    {
                        m_delayedDestroyed.eraseAtSwapBack( i );
                        --i;
                    }
                    else if( m_frame < destroyInfo.frame || ( m_frame - destroyInfo.frame ) >= kMaxFrameDistance )
                    {
                        handlesToDestroy.pushBack( destroyInfo );

                        m_delayedDestroyed.eraseAtSwapBack( i );
                        --i;
                    }
                }

                // destroy old handles
                if( !handlesToDestroy.isEmpty() )
                {
                    // sort delayed destroy handles by how they were allocated
                    handlesToDestroy.sort( []( const auto& a, const auto& b )
                    {
                        return zp_cmp( a.order, b.order );
                    } );

                    // destroy each delayed destroy m_handle
                    for( const auto& delayedDestroy : handlesToDestroy )
                    {
                        ProcessDelayedDestroy( delayedDestroy );
                    }
                }
            }
        }

        void VulkanContext::ForceFlushDestroyQueue()
        {
            ZP_PROFILE_CPU_BLOCK();

            for( auto& delayedDestroy : m_delayedDestroyed )
            {
                ProcessDelayedDestroy( delayedDestroy );
            }

            m_delayedDestroyed.clear();
        }

        StagingBufferAllocation VulkanContext::Allocate( zp_size_t size )
        {
            ZP_PROFILE_CPU_BLOCK();

            StagingBuffer& stagingBuffer = m_stagingBuffers[ m_frameIndex ];
            const zp_size_t allocationSize = ZP_ALIGN_SIZE( size, stagingBuffer.alignment );

            ZP_ASSERT( ( stagingBuffer.position + allocationSize ) < stagingBuffer.size );

            const StagingBufferAllocation allocation {
                .vkBuffer = stagingBuffer.vkBuffer,
                .vkDeviceMemory = stagingBuffer.vkDeviceMemory,
                .vkAccess = VK_ACCESS_2_HOST_WRITE_BIT,
                .offset = stagingBuffer.position,
                .size = allocationSize
            };

            stagingBuffer.position += allocationSize;

            return allocation;
        };

        BufferHandle VulkanContext::RequestBuffer( zp_size_t size )
        {
            ZP_PROFILE_CPU_BLOCK();

            const zp_bool_t allowTransferFrom = true;
            const zp_bool_t uniformBuffer = false;

            VkBufferUsageFlags vkBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

            if( allowTransferFrom )
            {
                vkBufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            }

            if( uniformBuffer )
            {
                vkBufferUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }

            switch( 0 )
            {
                case 0:
                    vkBufferUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                    break;

                case 1:
                    vkBufferUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                    break;

                case 2:
                    vkBufferUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
                    break;

            };

            const zp_size_t alignment = uniformBuffer ? 16 : 4;
            const zp_size_t alignedSize = ZP_ALIGN_SIZE( size, alignment );

            const VkBufferCreateInfo bufferCreateInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = alignedSize,
                .usage = vkBufferUsage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };

            VkBuffer vkBuffer {};
            HR( vkCreateBuffer( m_vkLocalDevice, &bufferCreateInfo, &m_vkAllocationCallbacks, &vkBuffer ) );

            // allocate memory
            VkMemoryRequirements memoryRequirements {};
            vkGetBufferMemoryRequirements( m_vkLocalDevice, vkBuffer, &memoryRequirements );

            const VkMemoryAllocateInfo memoryAllocateInfo {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memoryRequirements.size,
                .memoryTypeIndex = FindMemoryTypeIndex( m_vkPhysicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ),
            };

            VkDeviceMemory vkDeviceMemory {};
            HR( vkAllocateMemory( m_vkLocalDevice, &memoryAllocateInfo, &m_vkAllocationCallbacks, &vkDeviceMemory ) );

            HR( vkBindBufferMemory( m_vkLocalDevice, vkBuffer, vkDeviceMemory, 0 ) );

            SetDebugObjectName( m_vkInstance, m_vkLocalDevice, vkBuffer, "Buffer (%d)", memoryAllocateInfo.allocationSize );

            zp_uint32_t index;
            if( m_vkFreeBuffers.isEmpty() )
            {
                index = m_vkBuffers.length();
                m_vkBuffers.pushBackEmpty();
            }
            else
            {
                index = m_vkFreeBuffers.dequeue();
            }

            zp_hash32_t hash = zp_fnv32_1a( m_frame );
            hash = zp_fnv32_1a( index, hash );
            hash = zp_fnv32_1a( memoryAllocateInfo.allocationSize, hash );
            hash = zp_fnv32_1a( vkBufferUsage, hash );

            m_vkBuffers[ index ] = VulkanBuffer {
                .vkBuffer = vkBuffer,
                .vkDeviceMemory = vkDeviceMemory,
                .vkBufferUsage = vkBufferUsage,
                .vkAccess = VK_ACCESS_2_NONE,
                .offset = 0,
                .alignment = memoryRequirements.alignment,
                .size = memoryAllocateInfo.allocationSize,
                .hash = hash,
            };

            return { .index = index, .hash = hash };
        }

        void VulkanContext::ReleaseBuffer( BufferHandle bufferHandle )
        {
            ZP_PROFILE_CPU_BLOCK();

            // remove bufferHandle and clear out data
            const VulkanBuffer buffer = m_vkBuffers[ bufferHandle.index ];
            ZP_ASSERT( buffer.hash == bufferHandle.hash );
            m_vkBuffers[ bufferHandle.index ] = {};

            // add to free list
            m_vkFreeBuffers.enqueue( bufferHandle.index );

            // destroy underlying data
            QueueDestroy( buffer.vkDeviceMemory );

            QueueDestroy( buffer.vkBuffer );
        }

        void VulkanContext::UpdateBufferData( CommandQueueHandle commandQueueHandle, BufferHandle dstBufferHandle, zp_size_t dstOffset, Memory srcData )
        {
            ZP_PROFILE_CPU_BLOCK();

            StagingBufferAllocation allocation = Allocate( srcData.size );

            const VulkanCommandQueue& commandQueue = m_vkCommandQueues[ commandQueueHandle.index ];
            ZP_ASSERT( commandQueue.hash == commandQueueHandle.hash );

            VulkanBuffer& dstBuffer = m_vkBuffers[ dstBufferHandle.index ];
            ZP_ASSERT( dstBuffer.hash == dstBufferHandle.hash );

            // copy srcData to staging vkBuffer
            {
                void* dstMemory {};
                HR( vkMapMemory( m_vkLocalDevice, allocation.vkDeviceMemory, allocation.offset, allocation.size, 0, &dstMemory ) );

                zp_memcpy( dstMemory, allocation.size, srcData.ptr, srcData.size );

                vkUnmapMemory( m_vkLocalDevice, allocation.vkDeviceMemory );

                const VkMappedMemoryRange flushMemoryRange {
                    .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                    .memory = allocation.vkDeviceMemory,
                    .offset = allocation.offset,
                    .size = allocation.size,
                };
                HR( vkFlushMappedMemoryRanges( m_vkLocalDevice, 1, &flushMemoryRange ) );

                HR( vkInvalidateMappedMemoryRanges( m_vkLocalDevice, 1, &flushMemoryRange ) );
            }

            // transfer from CPU to GPU memory
            {
#if 0
                VkBufferMemoryBarrier memoryBarrier[2] {
                    {
                        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                        .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                        .buffer = allocation.vkBuffer,
                        .offset = allocation.offset,
                        .size = allocation.size,
                    },
                    {
                        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                        .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                        .buffer = dstBuffer.vkBuffer,
                        .offset = dstBuffer.offset,
                        .size = allocation.size,

                    }
                };

                vkCmdPipelineBarrier(
                    commandQueue.vkCommandBuffer,
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
#endif

                //CmdTransitionBufferAccess( commandQueue.vkCommandBuffer, allocation.vkBuffer, allocation.offset, allocation.size, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT );
                //CmdTransitionBufferAccess( commandQueue.vkCommandBuffer, dstBuffer.vkBuffer, dstBuffer.offset, allocation.size, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT );
                CmdTransitionBufferAccess( commandQueue.vkCommandBuffer, allocation.vkBuffer, allocation.offset, allocation.size, allocation.vkAccess, VK_ACCESS_2_TRANSFER_READ_BIT );
                CmdTransitionBufferAccess( commandQueue.vkCommandBuffer, dstBuffer, 0, allocation.size, VK_ACCESS_2_TRANSFER_WRITE_BIT );
            }

            // copy staging vkBuffer to dst vkBuffer
            {
                const VkBufferCopy2 region {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                    .srcOffset = allocation.offset,
                    .dstOffset = dstBuffer.offset,
                    .size = allocation.size,
                };

                const VkCopyBufferInfo2 copyBufferInfo {
                    .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                    .srcBuffer = allocation.vkBuffer,
                    .dstBuffer = dstBuffer.vkBuffer,
                    .regionCount = 1,
                    .pRegions = &region,
                };

                vkCmdCopyBuffer2( commandQueue.vkCommandBuffer, &copyBufferInfo );
            }

            // finalize dstbuffer upload
            {
                CmdTransitionBufferAccess( commandQueue.vkCommandBuffer, dstBuffer, 0, allocation.size, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT );
                CmdTransitionBufferAccess( commandQueue.vkCommandBuffer, allocation.vkBuffer, allocation.offset, allocation.size, allocation.vkAccess, VK_ACCESS_2_HOST_WRITE_BIT );
            }
        }

        TextureHandle VulkanContext::RequestTexture()
        {
            const VkFormat vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
            const VkImageType vkImageType = VK_IMAGE_TYPE_2D;
            const VkImageViewType vkImageViewType = VK_IMAGE_VIEW_TYPE_2D;
            const Size3Du size { .width = 64, .height = 64, .depth = 1 };
            const uint32_t mipLevels = 1;
            const zp_bool_t allowAutoMipGen = true;
            const zp_bool_t allowBlit = true;
            const VkImageAspectFlagBits vkImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
            const VkImageTiling vkImageTiling = VK_IMAGE_TILING_OPTIMAL;
            const VkImageLayout vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImageUsageFlags vkImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            if( allowAutoMipGen )
            {
                vkImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }

            if( allowBlit )
            {
                vkImageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }

#if ZP_DEBUG
            // make sure format is supported properly
            VkFormatProperties formatProperties {};
            vkGetPhysicalDeviceFormatProperties( m_vkPhysicalDevice, vkFormat, &formatProperties );

            const VkFormatFeatureFlags formatFeatures = vkImageTiling == VK_IMAGE_TILING_OPTIMAL ? formatProperties.optimalTilingFeatures : formatProperties.linearTilingFeatures;

            if( zp_flag32_all_set( vkImageUsageFlags, VK_IMAGE_USAGE_SAMPLED_BIT ) )
            {
                ZP_ASSERT( zp_flag32_all_set( formatFeatures, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) );
            }
#endif // ZP_DEBUG

            const VkImageCreateInfo imageCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .flags = 0,
                .imageType = vkImageType,
                .format = vkFormat,
                .extent {
                    .width = size.width,
                    .height = size.height,
                    .depth = size.depth,
                },
                .mipLevels = mipLevels,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = vkImageTiling,
                .usage = vkImageUsageFlags,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = vkImageLayout,
            };

            VkImage vkImage;
            HR( vkCreateImage( m_vkLocalDevice, &imageCreateInfo, &m_vkAllocationCallbacks, &vkImage ) );

            // allocate memory
            VkMemoryRequirements memoryRequirements {};
            vkGetImageMemoryRequirements( m_vkLocalDevice, vkImage, &memoryRequirements );

            const VkMemoryAllocateInfo memoryAllocateInfo {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memoryRequirements.size,
                .memoryTypeIndex = FindMemoryTypeIndex( m_vkPhysicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, Convert( ZP_MEMORY_PROPERTY_DEVICE_LOCAL ) ),
            };

            VkDeviceMemory vkDeviceMemory;
            HR( vkAllocateMemory( m_vkLocalDevice, &memoryAllocateInfo, &m_vkAllocationCallbacks, &vkDeviceMemory ) );

            HR( vkBindImageMemory( m_vkLocalDevice, vkImage, vkDeviceMemory, 0 ) );

            // create image views for reach mip level
            FixedArray<VkImageView, kMaxMipLevels> vkImageViews;
            FixedArray<VkImageLayout, kMaxMipLevels> vkImageLayouts;
            for( zp_uint32_t i = 0; i < mipLevels; ++i )
            {
                const VkImageViewCreateInfo imageViewCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = vkImage,
                    .viewType = vkImageViewType,
                    .format = vkFormat,
                    .components {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                    .subresourceRange {
                        .aspectMask = vkImageAspect,
                        .baseMipLevel = i,
                        .levelCount = mipLevels - i,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };

                HR( vkCreateImageView( m_vkLocalDevice, &imageViewCreateInfo, &m_vkAllocationCallbacks, &vkImageViews[ i ] ) );
                vkImageLayouts[ i ] = vkImageLayout;
            }

            zp_uint32_t index;
            if( m_vkFreeTextures.isEmpty() )
            {
                index = m_vkTextures.length();
                m_vkTextures.pushBackEmpty();
            }
            else
            {
                index = m_vkFreeTextures.dequeue();
            }

            zp_hash32_t hash = zp_fnv32_1a( m_frame );
            hash = zp_fnv32_1a( index, hash );
            hash = zp_fnv32_1a( memoryAllocateInfo.allocationSize, hash );
            hash = zp_fnv32_1a( size, hash );

            m_vkTextures[ index ] = VulkanTexture {
                .vkImage = vkImage,
                .vkImageViews = vkImageViews,
                .vkImageLayouts = vkImageLayouts,
                .vkDeviceMemory = vkDeviceMemory,
                .vkImageUsage = vkImageUsageFlags,
                .vkImageAspect = vkImageAspect,
                .vkFormat = vkFormat,
                .size = size,
                .mipCount = mipLevels,
                .loadedMipFlag = 0,
                .hash = hash,
            };

            return { .index = index, .hash = hash };
        }

        void VulkanContext::ReleaseTexture( TextureHandle textureHandle )
        {
            ZP_PROFILE_CPU_BLOCK();

            // clear texture
            const VulkanTexture texture = m_vkTextures[ textureHandle.index ];
            ZP_ASSERT( textureHandle.hash == texture.hash );
            m_vkTextures[ textureHandle.index ] = {};

            // queue freed index
            m_vkFreeTextures.enqueue( textureHandle.index );

            // destroy VK resources
            QueueDestroy( texture.vkDeviceMemory );

            for( zp_uint32_t i = 0; i < texture.mipCount; i++ )
            {
                QueueDestroy( texture.vkImageViews[ i ] );
            }

            QueueDestroy( texture.vkImage );
        }

        Size2Du CalculateMipSize( zp_uint32_t mipLevel, const Size2Du& size )
        {
            Size2Du mipSize = size;

            for( zp_uint32_t i = 0; i < mipLevel; ++i )
            {
                mipSize.width = zp_max( 1u, mipSize.width >> 1 );
                mipSize.height = zp_max( 1u, mipSize.height >> 1 );
            }

            return mipSize;
        }

        void VulkanContext::UpdateTextureMipData( CommandQueueHandle commandQueueHandle, TextureHandle dstTextureHandle, zp_uint32_t mipLevel, Memory srcData )
        {
            const VulkanCommandQueue& commandQueue = m_vkCommandQueues[ commandQueueHandle.index ];
            ZP_ASSERT( commandQueue.hash == commandQueue.hash );

            VulkanTexture& texture = m_vkTextures[ dstTextureHandle.index ];
            ZP_ASSERT( dstTextureHandle.hash == texture.hash );

            StagingBufferAllocation allocation = Allocate( srcData.size );

            // copy srcData to staging vkBuffer
            {
                void* dstMemory {};
                HR( vkMapMemory( m_vkLocalDevice, allocation.vkDeviceMemory, allocation.offset, allocation.size, 0, &dstMemory ) );

                zp_memcpy( dstMemory, allocation.size, srcData.ptr, srcData.size );

                vkUnmapMemory( m_vkLocalDevice, allocation.vkDeviceMemory );

                const VkMappedMemoryRange flushMemoryRange {
                    .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                    .memory = allocation.vkDeviceMemory,
                    .offset = allocation.offset,
                    .size = allocation.size,
                };
                HR( vkFlushMappedMemoryRanges( m_vkLocalDevice, 1, &flushMemoryRange ) );

                HR( vkInvalidateMappedMemoryRanges( m_vkLocalDevice, 1, &flushMemoryRange ) );
            }

            // tranfer staging buffer
            {
                CmdTransitionBufferAccess( commandQueue.vkCommandBuffer, allocation.vkBuffer, allocation.offset, allocation.size, allocation.vkAccess, VK_ACCESS_2_TRANSFER_READ_BIT );
            }

            const VkImageSubresourceRange subresourceRange {
                .aspectMask = texture.vkImageAspect,
                .baseMipLevel = mipLevel,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };

            // transition image to DST_OPT
            {
                CmdTransitionImageLayout( commandQueue.vkCommandBuffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange );
            }

            // copy buffer to texture
            {
                const Size2Du mipSize = CalculateMipSize( mipLevel, Size3Dto2D( texture.size ) );

                const VkBufferImageCopy2 region {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
                    .bufferOffset = allocation.offset,
                    .bufferRowLength = 0,
                    .bufferImageHeight = 0,
                    .imageSubresource {
                        .aspectMask = subresourceRange.aspectMask,
                        .mipLevel = mipLevel,
                        .baseArrayLayer = subresourceRange.baseArrayLayer,
                        .layerCount = subresourceRange.layerCount,
                    },
                    .imageOffset {
                        .x = 0,
                        .y = 0,
                        .z = 0,
                    },
                    .imageExtent {
                        .width = mipSize.width,
                        .height = mipSize.height,
                        .depth = 1,
                    },
                };

                const VkCopyBufferToImageInfo2 copyBufferToImageInfo {
                    .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
                    .srcBuffer = allocation.vkBuffer,
                    .dstImage = texture.vkImage,
                    .dstImageLayout = texture.vkImageLayouts[ mipLevel ],
                    .regionCount = 1,
                    .pRegions = &region,
                };

                vkCmdCopyBufferToImage2( commandQueue.vkCommandBuffer, &copyBufferToImageInfo );
            }

            // transition to READ_ONLY_OPT
            {
                CmdTransitionImageLayout( commandQueue.vkCommandBuffer, texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange );
                CmdTransitionBufferAccess( commandQueue.vkCommandBuffer, allocation.vkBuffer, allocation.offset, allocation.size, allocation.vkAccess, VK_ACCESS_2_HOST_WRITE_BIT );
            }
        }

        void VulkanContext::GenerateTextureMipLevels( CommandQueueHandle commandQueueHandle, TextureHandle textureHandle, zp_uint32_t mipLevelFlagsToGenerate )
        {
            VulkanTexture& texture = m_vkTextures[ textureHandle.index ];
            ZP_ASSERT( textureHandle.hash == texture.hash );

            // if there is no mips needed, or no mips loaded, or they are all loaded, return
            if( texture.mipCount == 1 || texture.loadedMipFlag == 0 || zp_flag32_all_set( texture.loadedMipFlag, mipLevelFlagsToGenerate ) )
            {
                return;
            }

            const VulkanCommandQueue& commandQueue = m_vkCommandQueues[ commandQueueHandle.index ];
            ZP_ASSERT( commandQueueHandle.hash == commandQueue.hash );

            // transition all mips to DST_OPT
            CmdTransitionImageLayout( commandQueue.vkCommandBuffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, {
                .aspectMask = texture.vkImageAspect,
                .baseMipLevel = 0,
                .levelCount = texture.mipCount,
                .baseArrayLayer = 0,
                .layerCount = 1,
            } );

            zp_int32_t width = static_cast<zp_int32_t>( texture.size.width );
            zp_int32_t height = static_cast<zp_int32_t>( texture.size.height );
            zp_int32_t mipWidth;
            zp_int32_t mipHeight;

            // generate each non-loaded mip
            for( zp_uint32_t i = 1; i < texture.mipCount; ++i )
            {
                mipWidth = zp_max( width >> 1, 1 );
                mipHeight = zp_max( height >> 1, 1 );

                const zp_uint32_t srcMip = i - 1;
                const zp_uint32_t dstMip = i;

                // if the src mip is loaded, then the dst can be loaded
                if( !zp_flag32_is_bit_set( texture.loadedMipFlag, srcMip ) )
                {
                    // mark mip as loaded
                    texture.loadedMipFlag |= 1 << dstMip;

                    // transition previous mip to SRC_OPT
                    CmdTransitionImageLayout( commandQueue.vkCommandBuffer, texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, {
                        .aspectMask = texture.vkImageAspect,
                        .baseMipLevel = srcMip,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    } );

                    // blip previous mip to current mip
                    const VkImageBlit2 region {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                        .srcSubresource {
                            .aspectMask = texture.vkImageAspect,
                            .mipLevel = srcMip,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                        .srcOffsets {
                            { 0,     0,      0 },
                            { width, height, 1 },
                        },
                        .dstSubresource {
                            .aspectMask = texture.vkImageAspect,
                            .mipLevel = dstMip,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                        .dstOffsets {
                            { 0,        0,         0 },
                            { mipWidth, mipHeight, 1 },
                        },
                    };

                    const VkBlitImageInfo2 blitImageInfo {
                        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                        .srcImage = texture.vkImage,
                        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        .dstImage = texture.vkImage,
                        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        .regionCount = 1,
                        .pRegions = &region,
                        .filter = VK_FILTER_LINEAR,
                    };

                    vkCmdBlitImage2( commandQueue.vkCommandBuffer, &blitImageInfo );

                    // transition previous mip to READ_ONLY_OPT
                    CmdTransitionImageLayout( commandQueue.vkCommandBuffer, texture, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, {
                        .aspectMask = texture.vkImageAspect,
                        .baseMipLevel = srcMip,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    } );
                }

                width = mipWidth;
                height = mipHeight;
            }

            // transition all mips to READ_ONLY_OPT
            CmdTransitionImageLayout( commandQueue.vkCommandBuffer, texture, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, {
                .aspectMask = texture.vkImageAspect,
                .baseMipLevel = 0,
                .levelCount = texture.mipCount,
                .baseArrayLayer = 0,
                .layerCount = 1,
            } );
        }

        ShaderHandle VulkanContext::RequestShader()
        {
            ZP_PROFILE_CPU_BLOCK();

            const MemoryArray<zp_uint32_t> shaderData {};

            const VkShaderModuleCreateInfo shaderModuleCreateInfo {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = shaderData.size(),
                .pCode = shaderData.data(),
            };

            VkShaderModule shaderModule;
            HR( vkCreateShaderModule( m_vkLocalDevice, &shaderModuleCreateInfo, &m_vkAllocationCallbacks, &shaderModule ) );

            //
            //
            //

            const Memory specializationData {};

            FixedVector<VkSpecializationMapEntry, 4> specializationMapEntries;
            specializationMapEntries.pushBack( {
                .constantID = 0,
                .offset = 0,
                .size = 0,
            } );

            const VkSpecializationInfo specializationInfo {
                .mapEntryCount = static_cast<zp_uint32_t>( specializationMapEntries.length() ),
                .pMapEntries = specializationMapEntries.data(),
                .dataSize = specializationData.size,
                .pData = specializationData.ptr,
            };

            constexpr const char* kDefaultShaderStageName[] {
                "vs_main",
                "tc_main",
                "te_main",
                "gs_main",
                "fs_main",
                "cs_main",
                "tm_main",
                "mm_main",
            };
            ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( kDefaultShaderStageName ) == ShaderStage_Count );

            zp_uint32_t shaderStageLoadedFlags = 1 << ZP_SHADER_STAGE_VERTEX | 1 << ZP_SHADER_STAGE_FRAGMENT;

            FixedVector<VkPipelineShaderStageCreateInfo, ShaderStage_Count> pipelineShaderStageCreateInfos;
            FixedVector<VkPushConstantRange, ShaderStage_Count> pushConstantRanges;
            for( zp_uint32_t i = 0; i < ShaderStage_Count; ++i )
            {
                if( zp_flag32_is_bit_set( shaderStageLoadedFlags, i ) )
                {
                    const VkShaderStageFlagBits stage = Convert( static_cast<ShaderStage>( i ) );

                    pipelineShaderStageCreateInfos.pushBack( {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = stage,
                        .module = shaderModule,
                        .pName = kDefaultShaderStageName[ i ],
                        .pSpecializationInfo = &specializationInfo,
                    } );

                    pushConstantRanges.pushBack( {
                        .stageFlags = stage,
                        .offset = 0,
                        .size = 0,
                    } );
                }
            }

            VkDescriptorSetLayoutBinding bings {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                .descriptorCount = 1,
                .stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            };

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
            };

            const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1,
                .pSetLayouts = &m_vkBindlessLayout,
                .pushConstantRangeCount = static_cast<zp_uint32_t>( pushConstantRanges.length() ),
                .pPushConstantRanges = pushConstantRanges.data(),
            };

            VkPipelineLayout vkPipelineLayout;
            HR( vkCreatePipelineLayout( m_vkLocalDevice, &pipelineLayoutCreateInfo, &m_vkAllocationCallbacks, &vkPipelineLayout ) );

            VkRenderPass vkRenderPass {};

#if USE_DYNAMIC_RENDERING
#else
            // collect all attachments
            FixedVector<VkAttachmentDescription2, 8> attachmentDescriptions;
            attachmentDescriptions.pushBack( {
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
                .format = VK_FORMAT_R8G8B8A8_SRGB,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_LOAD,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout    = VK_IMAGE_LAYOUT_UNDEFINED,
            } );

            // build subpasses
            FixedVector<VkAttachmentReference2, 8> colorAttachmentReferences;
            colorAttachmentReferences.pushBack( {
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_UNDEFINED,
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            } );

            FixedVector<VkAttachmentReference2, 8> inputAttachmentReferences {};

            VkAttachmentReference2 depthStencilAttachmentReference {
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_UNDEFINED,
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
            };

            FixedVector<VkSubpassDescription2, 8> subpassDescriptions;
            subpassDescriptions.pushBack( {
                .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount = static_cast<zp_uint32_t>( inputAttachmentReferences.length() ),
                .pInputAttachments = inputAttachmentReferences.data(),
                .colorAttachmentCount = static_cast<zp_uint32_t>( colorAttachmentReferences.length() ),
                .pColorAttachments = colorAttachmentReferences.data(),
                .pDepthStencilAttachment = &depthStencilAttachmentReference,
            } );

            FixedVector<VkSubpassDependency2, 8> subpassDependencies;
            subpassDependencies.pushBack( {
                .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                .srcSubpass = 0,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            } );

            const VkRenderPassCreateInfo2 renderPassCreateInfo {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
                .attachmentCount = static_cast<zp_uint32_t>( attachmentDescriptions.length() ),
                .pAttachments = attachmentDescriptions.data(),
                .subpassCount = static_cast<zp_uint32_t>( subpassDescriptions.length() ),
                .pSubpasses = subpassDescriptions.data(),
                .dependencyCount = static_cast<zp_uint32_t>( subpassDependencies.length() ),
                .pDependencies = subpassDependencies.data(),
            };

            HR( vkCreateRenderPass2( m_vkLocalDevice, &renderPassCreateInfo, &m_vkAllocationCallbacks, &vkRenderPass ) );
#endif

            VkPipeline vkBasePipeline {};

#if USE_DYNAMIC_RENDERING
            FixedVector<VkFormat, 8> colorAttachmentFormats;
            colorAttachmentFormats.pushBack( VK_FORMAT_UNDEFINED );
            VkFormat depthFormat = VK_FORMAT_UNDEFINED;
            VkFormat stencilFormat = VK_FORMAT_UNDEFINED;

            const VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .viewMask = 0,
                .colorAttachmentCount = static_cast<zp_uint32_t>( colorAttachmentFormats.length() ),
                .pColorAttachmentFormats = colorAttachmentFormats.data(),
                .depthAttachmentFormat = depthFormat,
                .stencilAttachmentFormat = stencilFormat,
            };
#endif

            // TODO: load from shader file
            FixedArray vertexInputBindingDescriptions {
                VkVertexInputBindingDescription {
                    .binding = 0,
                    .stride = sizeof( Vector3f ),
                    .inputRate  = VK_VERTEX_INPUT_RATE_VERTEX,
                },
                VkVertexInputBindingDescription {
                    .binding = 1,
                    .stride = sizeof( Vector2f ),
                    .inputRate  = VK_VERTEX_INPUT_RATE_VERTEX,
                },
            };
            FixedArray vertexInputAttributeDescriptions {
                VkVertexInputAttributeDescription {
                    .location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = 0,
                },
                VkVertexInputAttributeDescription {
                    .location = 0,
                    .binding = 1,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = 0,
                },
            };

            const VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = vertexInputBindingDescriptions.length(),
                .pVertexBindingDescriptions = vertexInputBindingDescriptions.data(),
                .vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.length(),
                .pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data(),
            };

            const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = false,
            };

            const VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .rasterizerDiscardEnable = true,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_BACK_BIT,
                .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            };

            const VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            };

            const VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .depthTestEnable = true,
                .depthWriteEnable = true,
                .depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL,
                .depthBoundsTestEnable = false,
                .stencilTestEnable = false,
                .front {},
                .back {},
            };

            const FixedArray blendStates {
                VkPipelineColorBlendAttachmentState {
                    .blendEnable = false,
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
                },
            };
            const VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = false,
                .logicOp = VK_LOGIC_OP_SET,
                .attachmentCount = static_cast<zp_uint32_t>( blendStates.length() ),
                .pAttachments = blendStates.data(),
                .blendConstants {
                    1, 1, 1, 1
                },
            };

            const FixedArray dynamicStates {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
            };

            const VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = static_cast<zp_uint32_t>( dynamicStates.length() ),
                .pDynamicStates = dynamicStates.data(),
            };

            const VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
#if USE_DYNAMIC_RENDERING
                .pNext = &pipelineRenderingCreateInfo,
#endif
                .stageCount = static_cast<zp_uint32_t>( pipelineShaderStageCreateInfos.length() ),
                .pStages = pipelineShaderStageCreateInfos.data(),
                .pVertexInputState = &pipelineVertexInputStateCreateInfo,
                .pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
                .pRasterizationState = &pipelineRasterizationStateCreateInfo,
                .pMultisampleState = &pipelineMultisampleStateCreateInfo,
                .pDepthStencilState = &pipelineDepthStencilStateCreateInfo,
                .pColorBlendState = &pipelineColorBlendStateCreateInfo,
                .pDynamicState = &pipelineDynamicStateCreateInfo,
                .layout = vkPipelineLayout,
                .renderPass = vkRenderPass,
                .subpass = 0,
                .basePipelineHandle = vkBasePipeline,
                .basePipelineIndex = -1,
            };

            VkPipeline vkPipeline;
            HR( vkCreateGraphicsPipelines( m_vkLocalDevice, m_vkPipelineCache, 1, &graphicsPipelineCreateInfo, &m_vkAllocationCallbacks, &vkPipeline ) );

            vkDestroyShaderModule( m_vkLocalDevice, shaderModule, &m_vkAllocationCallbacks );

            zp_uint32_t index {};

            zp_hash32_t hash = zp_fnv32_1a( m_frame );
            hash = zp_fnv32_1a( index, hash );

            return { .index = index, .hash = hash };
        }

        void VulkanContext::ReleaseShader( ShaderHandle shader )
        {

        }

        RenderPassHandle VulkanContext::RequestRenderPass()
        {
            zp_uint32_t index {};

            zp_hash32_t hash = zp_fnv32_1a( m_frame );
            hash = zp_fnv32_1a( index, hash );

            return { .index = index, .hash = hash };
        }

        void VulkanContext::ReleaseRenderPass( RenderPassHandle rendrPassHandle )
        {
        }

        void VulkanContext::BeginRenderPass( const CommandBeginRenderPass& cmd, MemoryArray<CommandBeginRenderPass::ColorAttachment> colorAttachments )
        {
            ZP_PROFILE_CPU_BLOCK();

            const VulkanCommandQueue& commandQueue = m_vkCommandQueues[ cmd.cmdQueue.index ];
            ZP_ASSERT( commandQueue.hash == cmd.cmdQueue.hash );

            // get size of target based on attachments
            Size2Du targetSize;
            if( !colorAttachments.empty() )
            {
                const VulkanRenderTarget& rt = m_vkRenderTargets[ colorAttachments[ 0 ].attachment.index ];
                ZP_ASSERT( rt.hash == colorAttachments[ 0 ].attachment.hash );

                targetSize = Size3Dto2D( rt.size );
            }
            else if( cmd.depthAttachment.attachment.valid() )
            {
                const VulkanRenderTarget& rt = m_vkRenderTargets[ cmd.depthAttachment.attachment.index ];
                ZP_ASSERT( rt.hash == cmd.depthAttachment.attachment.hash );

                targetSize = Size3Dto2D( rt.size );
            }
            else if( cmd.stencilAttachment.attachment.valid() )
            {
                const VulkanRenderTarget& rt = m_vkRenderTargets[ cmd.stencilAttachment.attachment.index ];
                ZP_ASSERT( rt.hash == cmd.stencilAttachment.attachment.hash );

                targetSize = Size3Dto2D( rt.size );
            }
            else
            {
                ZP_INVALID_CODE_PATH();
            }

            Rect2Du renderArea = cmd.renderArea;
            if( renderArea.size.width == 0 )
            {
                renderArea.size.width = targetSize.width;
            }
            if( renderArea.size.height == 0 )
            {
                renderArea.size.height = targetSize.height;
            }

#if USE_DYNAMIC_RENDERING
            FixedVector<VkRenderingAttachmentInfo, 8> colorAttachmentInfos;
            for( const auto& colorAttachment : colorAttachments )
            {
                const VulkanRenderTarget& colorRT = m_vkRenderTargets[ colorAttachment.attachment.index ];
                ZP_ASSERT( colorRT.hash == colorAttachment.attachment.hash );

                colorAttachmentInfos.pushBack( {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .imageView = colorRT.vkImageViews[ m_frameIndex ],
                    .imageLayout = colorRT.vkImageLayouts[ m_frameIndex ],
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue {
                        .color {
                            .float32 {
                                colorAttachment.clearColor.r,
                                colorAttachment.clearColor.g,
                                colorAttachment.clearColor.b,
                                colorAttachment.clearColor.a
                            }
                        }
                    },
                } );
            }

            const VulkanRenderTarget& depthStencilRT = m_vkRenderTargets[ cmd.depthAttachment.attachment.index ];
            ZP_ASSERT( depthStencilRT.hash == cmd.depthAttachment.attachment.hash );

            const VkRenderingAttachmentInfo depthStencilAttachment {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = depthStencilRT.vkImageViews[ m_frameIndex ],
                .imageLayout = depthStencilRT.vkImageLayouts[ m_frameIndex ],
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue {
                    .depthStencil {
                        .depth = cmd.depthAttachment.clearDepth,
                        .stencil = cmd.stencilAttachment.clearStencil,
                    }
                },
            };

            const VulkanRenderTarget& depthRT = m_vkRenderTargets[ cmd.depthAttachment.attachment.index ];
            ZP_ASSERT( depthRT.hash == cmd.depthAttachment.attachment.hash );

            const VkRenderingAttachmentInfo depthAttachment {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = depthRT.vkImageViews[ m_frameIndex ],
                .imageLayout = depthRT.vkImageLayouts[ m_frameIndex ],
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue {
                    .depthStencil {
                        .depth = cmd.depthAttachment.clearDepth,
                    }
                },
            };

            const VulkanRenderTarget& stencilRT = m_vkRenderTargets[ cmd.stencilAttachment.attachment.index ];
            ZP_ASSERT( stencilRT.hash == cmd.stencilAttachment.attachment.hash );

            const VkRenderingAttachmentInfo stencilAttachment {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = stencilRT.vkImageViews[ m_frameIndex ],
                .imageLayout = stencilRT.vkImageLayouts[ m_frameIndex ],
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue {
                    .depthStencil {
                        .stencil = cmd.stencilAttachment.clearStencil,
                    }
                },
            };

            const zp_bool_t useDepthStencil = cmd.depthAttachment.attachment == cmd.stencilAttachment.attachment;

            const VkRenderingInfo renderingInfo {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea {
                    .offset {
                        .x = static_cast<zp_int32_t>( renderArea.offset.x ),
                        .y = static_cast<zp_int32_t>( renderArea.offset.y ),
                    },
                    .extent {
                        .width = renderArea.size.width,
                        .height = renderArea.size.height,
                    },
                },
                .layerCount = 1,
                .viewMask = 0,
                .colorAttachmentCount = static_cast<zp_uint32_t>( colorAttachmentInfos.length() ),
                .pColorAttachments = colorAttachmentInfos.data(),
                .pDepthAttachment = useDepthStencil ? &depthStencilAttachment : &depthAttachment,
                .pStencilAttachment = useDepthStencil ? nullptr : &stencilAttachment,
            };

            vkCmdBeginRendering( commandQueue.vkCommandBuffer, &renderingInfo );
#else
            FixedVector<VkClearValue, 8> clearValues;
            clearValues.pushBack( {
                .color { 0, 0, 0, 0 }
            } );

            const VkRenderPassBeginInfo renderPassBeginInfo {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = renderPass.vkRenderPass,
                .framebuffer = renderPass.vkFramebuffer,
                .renderArea {
                    .offset {
                        .x = renderArea.offset.x,
                        .y = renderArea.offset.y,
                    },
                    .extent {
                        .width = renderArea.size.width,
                        .height = renderArea.size.height,
                    },
                },
                .clearValueCount = static_cast<zp_uint32_t>( clearValues.length() ),
                .pClearValues = clearValues.data(),
            };

            const VkSubpassBeginInfo subpassBeginInfo {
                .sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
                .contents = VK_SUBPASS_CONTENTS_INLINE,
            };

            vkCmdBeginRenderPass2( commandQueue.vkCommandBuffer, &renderPassBeginInfo, &subpassBeginInfo );
#endif
        }

        void VulkanContext::NextSubpass( const CommandNextSubpass& cmd )
        {
            ZP_PROFILE_CPU_BLOCK();

            const VulkanCommandQueue& commandQueue = m_vkCommandQueues[ cmd.cmdQueue.index ];
            ZP_ASSERT( commandQueue.hash == cmd.cmdQueue.hash );

#if USE_DYNAMIC_RENDERING
            const VkMemoryBarrier2 memoryBarrier {
                .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT,
            };

            const VkDependencyInfo dependencyInfo {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                .memoryBarrierCount = 1,
                .pMemoryBarriers = &memoryBarrier,
            };

            vkCmdPipelineBarrier2( commandQueue.vkCommandBuffer, &dependencyInfo );
#else
            const VkSubpassBeginInfo subpassBeginInfo {
                .sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
                .contents = VK_SUBPASS_CONTENTS_INLINE,
            };

            const VkSubpassEndInfo subpassEndInfo {
                .sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
            };

            vkCmdNextSubpass2( commandQueue.vkCommandBuffer, &subpassBeginInfo, &subpassEndInfo );
#endif
        }

        void VulkanContext::EndRenderPass( const CommandEndRenderPass& cmd )
        {
            ZP_PROFILE_CPU_BLOCK();

            const VulkanCommandQueue& commandQueue = m_vkCommandQueues[ cmd.cmdQueue.index ];
            ZP_ASSERT( commandQueue.hash == cmd.cmdQueue.hash );

#if USE_DYNAMIC_RENDERING
            vkCmdEndRendering( commandQueue.vkCommandBuffer );
#else
            const VkSubpassEndInfo subpassEndInfo {
                .sType = VK_STRUCTURE_TYPE_SUBPASS_END_INFO,
            };

            vkCmdEndRenderPass2( commandQueue.vkCommandBuffer, &subpassEndInfo );
#endif
        }

        void VulkanContext::Dispatch( const CommandDispatch& cmd )
        {
            ZP_PROFILE_CPU_BLOCK();
            ZP_PROFILE_GPU_STATS_DISPATCH( 1 );

            const VulkanCommandQueue& commandQueue = m_vkCommandQueues[ cmd.cmdQueue.index ];
            ZP_ASSERT( commandQueue.hash == cmd.cmdQueue.hash );

            vkCmdDispatch( commandQueue.vkCommandBuffer, cmd.groupCountX, cmd.groupCountY, cmd.groupCountZ );
        }

        void VulkanContext::DispatchIndirect( const CommandDispatchIndirect& cmd )
        {
            ZP_PROFILE_CPU_BLOCK();
            ZP_PROFILE_GPU_STATS_DISPATCH( 1 );

            const VulkanCommandQueue& commandQueue = m_vkCommandQueues[ cmd.cmdQueue.index ];
            ZP_ASSERT( commandQueue.hash == cmd.cmdQueue.hash );

            const VulkanBuffer& indirectBuffer = m_vkBuffers[ cmd.buffer.index ];
            ZP_ASSERT( indirectBuffer.hash == cmd.buffer.hash );

            vkCmdDispatchIndirect( commandQueue.vkCommandBuffer, indirectBuffer.vkBuffer, indirectBuffer.offset + cmd.offset );
        }

        void VulkanContext::Draw( const CommandDraw& cmd )
        {
            ZP_PROFILE_CPU_BLOCK();
            ZP_PROFILE_GPU_STATS_DRAW( vertexCount, instanceCount );

            const VulkanCommandQueue& commandQueue = m_vkCommandQueues[ cmd.cmdQueue.index ];
            ZP_ASSERT( commandQueue.hash == cmd.cmdQueue.hash );

            vkCmdDraw( commandQueue.vkCommandBuffer, cmd.vertexCount, cmd.instanceCount, cmd.firstVertex, cmd.firstInstance );
        }

        void VulkanContext::Blit( const CommandBlit& cmd )
        {
            ZP_PROFILE_CPU_BLOCK();

            const VulkanCommandQueue& commandQueue = m_vkCommandQueues[ cmd.cmdQueue.index ];
            ZP_ASSERT( commandQueue.hash == cmd.cmdQueue.hash );

            VulkanTexture& srcTexture = m_vkTextures[ cmd.srcTexture.index ];
            ZP_ASSERT( srcTexture.hash == cmd.srcTexture.hash );

            VulkanTexture& dstTexture = m_vkTextures[ cmd.dstTexture.index ];
            ZP_ASSERT( dstTexture.hash == cmd.dstTexture.hash );

            // original image layouts
            const VkImageLayout srcLayout = srcTexture.vkImageLayouts[ cmd.srcMip ];
            const VkImageLayout dstLayout = dstTexture.vkImageLayouts[ cmd.dstMip ];

            // blit subregions
            const VkImageSubresourceRange srcImageSubresourceRange {
                .aspectMask = srcTexture.vkImageAspect,
                .baseMipLevel = cmd.srcMip,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };
            const VkImageSubresourceRange dstImageSubresourceRange {
                .aspectMask = dstTexture.vkImageAspect,
                .baseMipLevel = cmd.dstMip,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };

            // transition images to proper transition layouts for blit
            {
                CmdTransitionImageLayout( commandQueue.vkCommandBuffer, srcTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcImageSubresourceRange );
                CmdTransitionImageLayout( commandQueue.vkCommandBuffer, dstTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstImageSubresourceRange );
            }

            // blit
            {
                Rect2Di srcRegion = cmd.srcRegion;
                Rect2Di dstRegion = cmd.dstRegion;

                // if src region size is -1, set to size of texture
                if( srcRegion.size.width < 0 )
                {
                    srcRegion.size.width = srcTexture.size.width;
                }
                if( srcRegion.size.height < 0 )
                {
                    srcRegion.size.height = srcTexture.size.height;
                }

                // if dst region size is -1, set to size of texture
                if( dstRegion.size.width < 0 )
                {
                    dstRegion.size.width = dstTexture.size.width;
                }
                if( dstRegion.size.height < 0 )
                {
                    dstRegion.size.height = dstTexture.size.height;
                }

                const VkImageBlit2 region {
                    .srcSubresource {
                        .aspectMask = srcTexture.vkImageAspect,
                        .mipLevel = cmd.srcMip,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                    .srcOffsets {
                        { .x = srcRegion.min().x, .y = srcRegion.min().y, .z = 1 },
                        { .x = srcRegion.max().x, .y = srcRegion.max().y, .z = 1 },
                    },
                    .dstSubresource {
                        .aspectMask = srcTexture.vkImageAspect,
                        .mipLevel = cmd.dstMip,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                    .dstOffsets {
                        { .x = dstRegion.min().x, .y = dstRegion.min().y, .z = 1 },
                        { .x = dstRegion.max().x, .y = dstRegion.max().y, .z = 1 },
                    },
                };

                const VkBlitImageInfo2 blitImageInfo {
                    .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                    .srcImage = srcTexture.vkImage,
                    .srcImageLayout = srcTexture.vkImageLayouts[ cmd.srcMip ],
                    .dstImage = dstTexture.vkImage,
                    .dstImageLayout = dstTexture.vkImageLayouts[ cmd.dstMip ],
                    .regionCount = 1,
                    .pRegions = &region,
                    .filter = VK_FILTER_LINEAR,
                };

                vkCmdBlitImage2( commandQueue.vkCommandBuffer, &blitImageInfo );
            }

            // transition images back to what layout they were
            {
                CmdTransitionImageLayout( commandQueue.vkCommandBuffer, srcTexture, srcLayout, srcImageSubresourceRange );
                CmdTransitionImageLayout( commandQueue.vkCommandBuffer, dstTexture, dstLayout, dstImageSubresourceRange );
            }
        }

        void VulkanContext::UseBuffer( BufferHandle bufferHandle )
        {
            ZP_PROFILE_CPU_BLOCK();

            const VulkanBuffer& buffer = m_vkBuffers[ bufferHandle.index ];
            ZP_ASSERT( buffer.hash == bufferHandle.hash );

            VkDescriptorSet vkDescriptorSet = m_vkBindlessDescriptorSets[ m_frameIndex ];

            const VkDescriptorBufferInfo bufferInfo {
                .buffer = buffer.vkBuffer,
                .offset = buffer.offset,
                .range = buffer.size,
            };

            zp_size_t writeUpdates = 0;
            FixedArray<VkWriteDescriptorSet, 2> descriptorSetWrites {};

            if( ( buffer.vkBufferUsage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ) == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT )
            {
                descriptorSetWrites[ writeUpdates ] = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = vkDescriptorSet,
                    .dstArrayElement = bufferHandle.index,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &bufferInfo,
                };
                ++writeUpdates;
            }

            if( ( buffer.vkBufferUsage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT ) == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT )
            {
                descriptorSetWrites[ writeUpdates ] = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = vkDescriptorSet,
                    .dstArrayElement = bufferHandle.index,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &bufferInfo,
                };
                ++writeUpdates;
            }

            vkUpdateDescriptorSets( m_vkLocalDevice, writeUpdates, descriptorSetWrites.data(), 0, nullptr );
        }

        void VulkanContext::UseTexture( TextureHandle textureHandle )
        {
            ZP_PROFILE_CPU_BLOCK();

            const VulkanTexture& texture = m_vkTextures[ textureHandle.index ];
            ZP_ASSERT( texture.hash == textureHandle.hash );

            const VulkanSampler sampler {};

            VkDescriptorSet vkDescriptorSet = m_vkBindlessDescriptorSets[ m_frameIndex ];

            // get the lowest loaded mip
            const zp_uint32_t lowestLoadedMip = zp_bitscan_forward( texture.loadedMipFlag );

            const VkDescriptorImageInfo imageInfo {
                .sampler = sampler.vkSampler,
                .imageView = texture.vkImageViews[ lowestLoadedMip ],
                .imageLayout = texture.vkImageLayouts[ lowestLoadedMip ],
            };

            const VkWriteDescriptorSet descriptorSetWrite {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = vkDescriptorSet,
                .dstArrayElement = textureHandle.index,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
            };

            vkUpdateDescriptorSets( m_vkLocalDevice, 1, &descriptorSetWrite, 0, nullptr );
        }

        void VulkanContext::QueueDestroy( void* handle, VkObjectType objectType )
        {
            ZP_PROFILE_CPU_BLOCK();

            const zp_size_t index = m_delayedDestroyed.pushBackEmptyRangeAtomic( 1, false );
            m_delayedDestroyed[ index ] = {
                .frame = m_frame,
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

            void AcquireNextImage( zp_size_t frameIndex );

            void Present( zp_size_t frameIndex );

            void Rebuild();

            VkSemaphore GetWaitSemaphore( zp_size_t frameIndex ) const
            {
                return m_vkSwapchainAcquireSemaphores[ frameIndex ];
            }

            VkSemaphore GetSignalSemaphore( zp_size_t frameIndex ) const
            {
                return m_vkRenderFinishedSemaphores[ frameIndex ];
            }

            VkFence GetFrameInFlightFence( zp_size_t frameIndex ) const
            {
                return m_vkFrameInFlightFences[ frameIndex ];
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

            FixedArray<VkImage, kMaxBufferedFrameCount> m_vkSwapchainImages;
            FixedArray<VkImageView, kMaxBufferedFrameCount> m_vkSwapchainImageViews;
            FixedArray<VkFramebuffer, kMaxBufferedFrameCount> m_vkSwapchainFrameBuffers;

            FixedArray<VkSemaphore, kMaxBufferedFrameCount> m_vkSwapchainAcquireSemaphores;
            FixedArray<VkSemaphore, kMaxBufferedFrameCount> m_vkRenderFinishedSemaphores;
            FixedArray<VkFence, kMaxBufferedFrameCount> m_vkFrameInFlightFences;
            FixedArray<VkFence, kMaxBufferedFrameCount> m_vkSwapchainImageAcquiredFences;
            FixedArray<zp_uint32_t, kMaxBufferedFrameCount> m_swapchainImageIndices;

            zp_size_t m_maxFramesInFlight;

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

        void VulkanSwapchain::AcquireNextImage( zp_size_t frameIndex )
        {
            ZP_PROFILE_CPU_BLOCK();

            if( m_requiresRebuild )
            {
                DestroySwapchain();
                CreateSwapchain();

                m_requiresRebuild = false;
            }

            HR( vkWaitForFences( m_context->GetLocalDevice(), 1, &m_vkFrameInFlightFences[ frameIndex ], VK_TRUE, zp_limit<zp_uint64_t>::max() ) );

            const VkResult acquireNextImageResult = vkAcquireNextImageKHR( m_context->GetLocalDevice(), m_vkSwapchain, zp_limit<zp_uint64_t>::max(), m_vkSwapchainAcquireSemaphores[ frameIndex ], VK_NULL_HANDLE, &m_swapchainImageIndices[ frameIndex ] );
            if( acquireNextImageResult != VK_SUCCESS )
            {
                switch( acquireNextImageResult )
                {
                    case VK_ERROR_OUT_OF_DATE_KHR:
                    case VK_SUBOPTIMAL_KHR:
                        m_requiresRebuild = true;
                        break;
                    default:
                        HR( acquireNextImageResult );
                        break;
                }
            }

            HR( vkResetFences( m_context->GetLocalDevice(), 1, &m_vkFrameInFlightFences[ frameIndex ] ) );
        }

        void VulkanSwapchain::Present( zp_size_t frameIndex )
        {
            ZP_PROFILE_CPU_BLOCK();

            VkResult swapchainResult {};
            const VkPresentInfoKHR presentInfo {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &m_vkRenderFinishedSemaphores[ frameIndex ],
                .swapchainCount = 1,
                .pSwapchains = &m_vkSwapchain,
                .pImageIndices = &m_swapchainImageIndices[ frameIndex ],
                .pResults = &swapchainResult,
            };

            const VkResult presentResult = vkQueuePresentKHR( m_context->GetQueues().present.vkQueue, &presentInfo );
            if( presentResult != VK_SUCCESS )
            {
                if( presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR )
                {
                    // rebuild swapchain
                }
                else
                {
                    HR( presentResult );
                }
            }
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
            const FixedArray<VkSurfaceFormatKHR, 2> preferredSurfaceFormats {
                VkSurfaceFormatKHR { .format = VK_FORMAT_B8G8R8A8_SNORM, .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR },
                VkSurfaceFormatKHR { .format = VK_FORMAT_R8G8B8A8_SNORM, .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR },
            };

            m_vkSwapChainFormat = ChooseSwapChainSurfaceFormat( supportedSurfaceFormats, preferredSurfaceFormats.asReadonly() );

            //
            // TODO: make this configurable?
            const zp_uint32_t preferredImageCount = kMaxBufferedFrameCount;
            uint32_t imageCount = zp_max( preferredImageCount, surfaceCapabilities.minImageCount );
            if( surfaceCapabilities.maxImageCount > 0 )
            {
                imageCount = zp_min( imageCount, surfaceCapabilities.maxImageCount );
            }

            m_maxFramesInFlight = imageCount;
            ZP_ASSERT_MSG_ARGS( m_maxFramesInFlight <= kMaxBufferedFrameCount, "Increase buffered frames %d <= %d", m_maxFramesInFlight, kMaxBufferedFrameCount );

            //
            const Queues& queues = m_context->GetQueues();
            const bool useConcurrentSharingMode = queues.graphics.familyIndex != queues.present.familyIndex;

            const FixedArray<zp_uint32_t, 2> indices {
                queues.graphics.familyIndex,
                queues.present.familyIndex,
            };

            const VkSwapchainCreateInfoKHR swapChainCreateInfo {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface = m_context->GetSurface(),
                .minImageCount = static_cast<zp_uint32_t>( m_maxFramesInFlight ),
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
                .oldSwapchain = m_vkSwapchain,
            };

            HR( vkCreateSwapchainKHR( m_context->GetLocalDevice(), &swapChainCreateInfo, m_context->GetAllocationCallbacks(), &m_vkSwapchain ) );

            SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), m_vkSwapchain, "Swapchain" );

            if( swapChainCreateInfo.oldSwapchain != VK_NULL_HANDLE )
            {
                m_context->QueueDestroy( swapChainCreateInfo.oldSwapchain );
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
                        .frameCount = m_currentFrameIndex,
                        .m_handle = m_swapchainData.vkSwapchainDefaultRenderPass,
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

                SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), m_vkSwapchainImageViews[ i ], "Swapchain Image View %d", i );

#if !USE_DYNAMIC_RENDERING
                //
                VkImageView attachments[] { m_vkSwapchainImageViews[ i ] };

                const VkFramebufferCreateInfo framebufferCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .renderPass = nullptr, //m_swapchainData.vkSwapchainDefaultRenderPass,
                    .attachmentCount = ZP_ARRAY_SIZE( attachments ),
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
#endif
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

                SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), m_vkSwapchainAcquireSemaphores[ i ], "Swapchain Acquire Semaphore %d", i );
                SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), m_vkRenderFinishedSemaphores[ i ], "Render Finished Semaphore %d", i );
                SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), m_vkFrameInFlightFences[ i ], "In Flight Fence %d", i );
                SetDebugObjectName( m_context->GetInstance(), m_context->GetLocalDevice(), m_vkSwapchainImageAcquiredFences[ i ], "Swapchain Image Acquire Fence %d", i );
            }

            // transition swapchain images to present layout
            VkCommandBuffer cmdBuffer = RequestSingleUseCommandBuffer( m_context->GetLocalDevice(), m_context->GetTransientCommandPool() );
            for( zp_size_t i = 0; i < m_maxFramesInFlight; ++i )
            {
                VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                CmdTransitionImageLayout( cmdBuffer, m_vkSwapchainImages[ i ], imageLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                } );
            }
            ReleaseSingleUseCommandBuffer( m_context->GetLocalDevice(), m_context->GetTransientCommandPool(), cmdBuffer, m_context->GetQueues().graphics.vkQueue, m_context->GetAllocationCallbacks() );
        }

        void VulkanSwapchain::DestroySwapchain()
        {
            ZP_ASSERT( m_context != nullptr );

            HR( vkDeviceWaitIdle( m_context->GetLocalDevice() ) );

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

            ~VulkanGraphicsDevice();

            void Initialize( const GraphicsDeviceDesc& graphicsDeviceDesc );

            void Destroy();

            TextureHandle RequestTexture();

            BufferHandle RequestBuffer( zp_size_t size );

            void BeginFrame( zp_size_t frameIndex );

            void ExecuteCommandBuffer( GraphicsCommandBuffer* cmdBuffer, JobHandle parentJob );

            void EndFrame();

            GraphicsCommandBuffer* SubmitAndRequestNewCommandBuffer( GraphicsCommandBuffer* cmdBuffer ) final;

        private:
            VulkanContext m_context;
            VulkanSwapchain m_swapchain;

            JobHandle m_frameJob;
            FixedArray<GraphicsCommandBuffer*, kMaxBufferedFrameCount> m_frameCommandBuffers;
            FixedQueue<GraphicsCommandBuffer*, kMaxBufferedFrameCount> m_submitQueue;


            zp_uint64_t m_frame;
            zp_size_t m_frameIndex;

        public:
            MemoryLabel memoryLabel;
        };

        VulkanGraphicsDevice::VulkanGraphicsDevice( MemoryLabel memoryLabel )
            : memoryLabel( memoryLabel )
        {
        }

        VulkanGraphicsDevice::~VulkanGraphicsDevice()
        {
        }

        void VulkanGraphicsDevice::Initialize( const GraphicsDeviceDesc& graphicsDeviceDesc )
        {
            for( zp_size_t i = 0; i < kMaxBufferedFrameCount; ++i )
            {
                m_frameCommandBuffers[ i ] = ZP_NEW_ARGS( memoryLabel, GraphicsCommandBuffer, graphicsDeviceDesc.commandBufferPageSize );
            }

            m_context.Initialize( graphicsDeviceDesc );

            m_swapchain.Initialize( &m_context, graphicsDeviceDesc.windowHandle );
        }

        void VulkanGraphicsDevice::Destroy()
        {
            m_swapchain.Destroy();

            m_context.Destroy();

            for( zp_size_t i = 0; i < kMaxBufferedFrameCount; ++i )
            {
                ZP_DELETE( GraphicsCommandBuffer, m_frameCommandBuffers[ i ] );
            }
        }

        BufferHandle VulkanGraphicsDevice::RequestBuffer( zp_size_t size )
        {
            return m_context.RequestBuffer( size );
        }

        struct SubmitCommandBufferJob
        {
            GraphicsCommandBuffer* cmdBuffer;
            VulkanGraphicsDevice* graphicsDevice;
            zp_uint64_t frame;

            static void Execute( const JobWorkArgs& args )
            {
                const SubmitCommandBufferJob* job = args.jobMemory.as<SubmitCommandBufferJob>();

                Log::info() << "Render Frame: " << job->frame << Log::endl;

                job->graphicsDevice->BeginFrame( job->frame );
                {
                    const JobHandle parentJob = JobSystem::PrepareEmpty();

                    job->graphicsDevice->ExecuteCommandBuffer( job->cmdBuffer, parentJob );

                    JobSystem::ScheduleBatchJobs();

                    JobSystem::Complete( parentJob );
                }
                job->graphicsDevice->EndFrame();
            }
        };

        GraphicsCommandBuffer* VulkanGraphicsDevice::SubmitAndRequestNewCommandBuffer( GraphicsCommandBuffer* cmdBuffer )
        {
            zp_size_t frameIndex = 0;
            zp_uint64_t nextFrame = 0;
            zp_size_t nextFrameIndex = 0;

            JobSystem::Complete( m_frameJob );

            if( cmdBuffer != nullptr )
            {
                frameIndex = cmdBuffer->frame % kMaxBufferedFrameCount;
                nextFrame = cmdBuffer->frame + 1;
                nextFrameIndex = nextFrame % kMaxBufferedFrameCount;

                m_frameJob = JobSystem::Run( SubmitCommandBufferJob { .cmdBuffer = cmdBuffer, .graphicsDevice = this, .frame = cmdBuffer->frame } );
            }

            //const VkFence frameInFlightFence = m_swapchain.GetFrameInFlightFence( nextFrameIndex );
            //HR( vkWaitForFences( m_context.GetLocalDevice(), 1, &frameInFlightFence, VK_TRUE, zp_limit<zp_uint64_t>::max() ) );

            GraphicsCommandBuffer* nextCmdBuffer = m_frameCommandBuffers[ nextFrameIndex ];
            nextCmdBuffer->frame = nextFrame;

            return nextCmdBuffer;
        }

        void VulkanGraphicsDevice::BeginFrame( zp_uint64_t frame )
        {
            m_frame = frame;
            m_frameIndex = frame % kMaxBufferedFrameCount;

            m_context.BeginFrame( m_frame, m_frameIndex );

            m_swapchain.AcquireNextImage( m_frameIndex );
        }

        void VulkanGraphicsDevice::ExecuteCommandBuffer( GraphicsCommandBuffer* cmdBuffer, JobHandle parentJob )
        {
            DataStreamReader dataStreamReader( cmdBuffer->Data() );

            while( !dataStreamReader.end() )
            {
                const CommandHeader* header = dataStreamReader.readPtr<CommandHeader>();

                switch( header->type )
                {
                    case CommandType::None:
                        break;

                    case CommandType::UpdateBufferData:
                    {
                        CommandUpdateBufferData updateBufferData {};
                        dataStreamReader.read( updateBufferData );

                        const Memory srcData = dataStreamReader.readMemory( updateBufferData.srcLength );
                        dataStreamReader.readAlignment( 8 );

                        m_context.UpdateBufferData( updateBufferData.cmdQueue, updateBufferData.dstBuffer, updateBufferData.dstOffset, srcData );
                    }
                        break;

                    case CommandType::UpdateBufferDataExternal:
                    {
                        CommandUpdateBufferDataExternal updateBufferDataExternal {};
                        dataStreamReader.read( updateBufferDataExternal );

                        m_context.UpdateBufferData( updateBufferDataExternal.cmdQueue, updateBufferDataExternal.dstBuffer, updateBufferDataExternal.dstOffset, updateBufferDataExternal.srcData );
                    }
                        break;

                    case CommandType::BeginRenderPass:
                    {
                        const CommandBeginRenderPass* beginRenderPass = dataStreamReader.readPtr<CommandBeginRenderPass>();
                        MemoryArray<CommandBeginRenderPass::ColorAttachment> colorAttachments = dataStreamReader.readMemoryArray<CommandBeginRenderPass::ColorAttachment>( beginRenderPass->colorAttachmentCount );

                        m_context.BeginRenderPass( *beginRenderPass, colorAttachments );
                    }
                        break;

                    case CommandType::Dispatch:
                    {
                        const CommandDispatch* dispatch = dataStreamReader.readPtr<CommandDispatch>();

                        m_context.Dispatch( *dispatch );
                    }
                        break;

                    case CommandType::Draw:
                    {
                        const CommandDraw* draw = dataStreamReader.readPtr<CommandDraw>();

                        m_context.Draw( *draw );
                    }

                    case CommandType::Blit:
                    {
                        const CommandBlit* blit = dataStreamReader.readPtr<CommandBlit>();

                        m_context.Blit( *blit );
                    }
                        break;

                    default:
                        ZP_INVALID_CODE_PATH_MSG( "Unknown CommandType" );
                        break;
                }
            }
        }

        void VulkanGraphicsDevice::EndFrame()
        {
            m_context.EndFrame( m_swapchain.GetWaitSemaphore( m_frameIndex ), m_swapchain.GetSignalSemaphore( m_frameIndex ), m_swapchain.GetFrameInFlightFence( m_frameIndex ) );

            m_swapchain.Present( m_frameIndex );
        }
    };

    //
    //
    //

    namespace internal
    {
        GraphicsDevice* CreateVulkanGraphicsDevice( MemoryLabel memoryLabel, const GraphicsDeviceDesc& desc )
        {
            VulkanGraphicsDevice* vk = ZP_NEW( memoryLabel, VulkanGraphicsDevice );
            vk->Initialize( desc );
            return vk;
        }

        void DestroyVulkanGraphicsDevice( GraphicsDevice* graphicsDevice )
        {
            VulkanGraphicsDevice* vk = reinterpret_cast<VulkanGraphicsDevice*>( graphicsDevice );
            vk->Destroy();

            ZP_SAFE_DELETE( VulkanGraphicsDevice, vk );
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

        // create staging dstbuffer
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
                    .frameCount = m_currentFrameIndex,
                    .m_handle = m_swapchainData.vkSwapchainDefaultRenderPass,
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
                    .frameCount = m_currentFrameIndex,
                    .m_handle = m_swapchainData.swapchainImageViews[ i ],
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
                    .frameCount = m_currentFrameIndex,
                    .m_handle = m_swapchainData.swapchainFrameBuffers[ i ],
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
        const zp_size_t perFrameStagingBufferSize = m_stagingBuffer.size / kMaxBufferedFrameCount;

        for( zp_size_t i = 0; i < kMaxBufferedFrameCount; ++i )
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
            //vkCmdWriteTimestamp( dstbuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, currentFrameData.vkTimestampQueryPool, preCommandQueueIndex * 2 + 1 );
            //vkCmdEndQuery( dstbuffer, currentFrameData.vkPipelineStatisticsQueryPool, 0 );
            //vkCmdResetQueryPool( static_cast<VkCommandBuffer>( currentFrameData.commandQueues[ preCommandQueueIndex ].commandBuffer ), currentFrameData.vkTimestampQueryPool, commandQueueCount * 2 + 2, 16 - (commandQueueCount * 2 + 2));
#endif
            //HR( vkEndCommandBuffer( dstbuffer ) );
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
            .frameCount = m_currentFrameIndex,
            .m_handle = renderPass->internalRenderPass,
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
            .frameCount = m_currentFrameIndex,
            .m_handle = graphicsPipelineState->pipelineState,
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
            .frameCount = m_currentFrameIndex,
            .m_handle = pipelineLayout->layout,
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
            .vkBufferUsage = Convert( graphicsBufferDesc.usageFlags ),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
        };

        VkBuffer dstbuffer;
        HR( vkCreateBuffer( m_vkLocalDevice, &bufferCreateInfo, &m_vkAllocationCallbacks, &dstbuffer ) );

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements( m_vkLocalDevice, dstbuffer, &memoryRequirements );

        VkMemoryAllocateInfo memoryAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = FindMemoryTypeIndex( m_vkPhysicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, Convert( graphicsBufferDesc.memoryPropertyFlags ) ),
        };

        VkDeviceMemory vkDeviceMemory;
        HR( vkAllocateMemory( m_vkLocalDevice, &memoryAllocateInfo, &m_vkAllocationCallbacks, &vkDeviceMemory ) );

        HR( vkBindBufferMemory( m_vkLocalDevice, dstbuffer, vkDeviceMemory, 0 ) );

        *graphicsBuffer = {
            .dstbuffer = dstbuffer,
            .vkDeviceMemory = vkDeviceMemory,
            .offset = 0,
            .size = memoryRequirements.size,
            .alignment = memoryRequirements.alignment,
            .usageFlags = graphicsBufferDesc.usageFlags,
            .isVirtualBuffer = false,
        };

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, graphicsBufferDesc.name, VK_OBJECT_TYPE_BUFFER, dstbuffer );
    }

    void VulkanGraphicsDevice::destroyBuffer( GraphicsBuffer* graphicsBuffer )
    {
        if( !graphicsBuffer->isVirtualBuffer )
        {
            const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 2, false );

            m_delayedDestroy[ index + 0 ] = {
                .frameCount = m_currentFrameIndex,
                .m_handle = graphicsBuffer->dstbuffer,
                .allocator = &m_vkAllocationCallbacks,
                .localDevice = m_vkLocalDevice,
                .instance = m_vkInstance,
                .order = index + 0,
                .type = DelayedDestroyType::Buffer
            };

            m_delayedDestroy[ index + 1 ] = {
                .frameCount = m_currentFrameIndex,
                .m_handle = graphicsBuffer->vkDeviceMemory,
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
            .frameCount = m_currentFrameIndex,
            .m_handle = shader->shaderHandle,
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
            .vkBufferUsage = Convert( textureCreateDesc->vkBufferUsage ),
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

        VkDeviceMemory vkDeviceMemory;
        HR( vkAllocateMemory( m_vkLocalDevice, &memoryAllocateInfo, &m_vkAllocationCallbacks, &vkDeviceMemory ) );

        HR( vkBindImageMemory( m_vkLocalDevice, image, vkDeviceMemory, 0 ) );

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
                .aspectMask = ConvertAspect( textureCreateDesc->vkBufferUsage ),
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
            .vkBufferUsage = textureCreateDesc->vkBufferUsage,
            .size = textureCreateDesc->size,
            .textureHandle = image,
            .textureViewHandle = imageView,
            .textureMemoryHandle = vkDeviceMemory,
        };

        SetDebugObjectName( m_vkInstance, m_vkLocalDevice, "Image View", VK_OBJECT_TYPE_IMAGE_VIEW, imageView );
    }

    void VulkanGraphicsDevice::destroyTexture( Texture* texture )
    {
        const zp_size_t index = m_delayedDestroy.pushBackEmptyRangeAtomic( 3, false );

        m_delayedDestroy[ index + 0 ] = {
            .frameCount = m_currentFrameIndex,
            .m_handle = texture->textureViewHandle,
            .allocator = &m_vkAllocationCallbacks,
            .localDevice = m_vkLocalDevice,
            .instance = m_vkInstance,
            .order = index + 0,
            .type = DelayedDestroyType::ImageView,
        };
        m_delayedDestroy[ index + 1 ] = {
            .frameCount = m_currentFrameIndex,
            .m_handle = texture->textureMemoryHandle,
            .allocator = &m_vkAllocationCallbacks,
            .localDevice = m_vkLocalDevice,
            .instance = m_vkInstance,
            .order = index + 1,
            .type = DelayedDestroyType::Memory,
        };
        m_delayedDestroy[ index + 2 ] = {
            .frameCount = m_currentFrameIndex,
            .m_handle = texture->textureHandle,
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
            .frameCount = m_currentFrameIndex,
            .m_handle = sampler->samplerHandle,
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
        VkDeviceMemory vkDeviceMemory = static_cast<VkDeviceMemory>( graphicsBuffer.vkDeviceMemory );
        HR( vkMapMemory( m_vkLocalDevice, vkDeviceMemory, offset + graphicsBuffer.offset, size, 0, memory ) );
    }

    void VulkanGraphicsDevice::unmapBuffer( const GraphicsBuffer& graphicsBuffer )
    {
        VkDeviceMemory vkDeviceMemory = static_cast<VkDeviceMemory>( graphicsBuffer.vkDeviceMemory );
        vkUnmapMemory( m_vkLocalDevice, vkDeviceMemory );
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
        commandQueue->frameCount = m_currentFrameIndex;

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

        VkCommandBuffer dstbuffer = static_cast<VkCommandBuffer>( commandQueue->commandBuffer );
        HR( vkBeginCommandBuffer( dstbuffer, &commandBufferBeginInfo ) );

#if ZP_USE_PROFILER
        if( queue != ZP_RENDER_QUEUE_TRANSFER )
        {
            //vkCmdResetQueryPool( dstbuffer, frameData.vkTimestampQueryPool, commandQueueIndex * 2, 2 );
            //vkCmdWriteTimestamp( dstbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameData.vkTimestampQueryPool, commandQueueIndex * 2 );
        }

        //vkCmdBeginQuery( dstbuffer, frameData.vkPipelineStatisticsQueryPool, 0, 0 );
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

        PerFrameData& frameData = getFrameData( commandQueue->frameCount );
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
        vkCmdBindIndexBuffer( static_cast<VkCommandBuffer>(commandQueue->commandBuffer), static_cast<VkBuffer>( graphicsBuffer->dstbuffer), bufferOffset, indexType );
    }

    void VulkanGraphicsDevice::bindVertexBuffers( zp_uint32_t firstBinding, zp_uint32_t bindingCount, const GraphicsBuffer** graphicsBuffers, zp_size_t* offsets, CommandQueue* commandQueue )
    {
        VkBuffer vertexBuffers[bindingCount];
        VkDeviceSize vertexBufferOffsets[bindingCount];

        for( zp_uint32_t i = 0; i < bindingCount; ++i )
        {
            vertexBuffers[ i ] = static_cast<VkBuffer>( graphicsBuffers[ i ]->dstbuffer );
            vertexBufferOffsets[ i ] = graphicsBuffers[ i ]->offset + ( offsets == nullptr ? 0 : offsets[ i ] );
        }

        vkCmdBindVertexBuffers( static_cast<VkCommandBuffer>(commandQueue->commandBuffer), firstBinding, bindingCount, vertexBuffers, vertexBufferOffsets );
    }

    void VulkanGraphicsDevice::updateTexture( const TextureUpdateDesc* textureUpdateDesc, const Texture* dstTexture, CommandQueue* commandQueue )
    {
        PerFrameData& frameData = getFrameData( commandQueue->frameCount );
        GraphicsBufferAllocation allocation = frameData.perFrameStagingBuffer.allocate( textureUpdateDesc->textureDataSize );

        auto commandBuffer = static_cast<VkCommandBuffer>( commandQueue->commandBuffer );
        const zp_uint32_t mipCount = textureUpdateDesc->maxMipLevel - textureUpdateDesc->minMipLevel;

        // copy data to staging dstbuffer
        {
            auto vkDeviceMemory = static_cast<VkDeviceMemory>( allocation.vkDeviceMemory );

            void* dstMemory {};
            HR( vkMapMemory( m_vkLocalDevice, vkDeviceMemory, allocation.offset, allocation.size, 0, &dstMemory ) );

            zp_memcpy( dstMemory, allocation.size, textureUpdateDesc->textureData, textureUpdateDesc->textureDataSize );

            vkUnmapMemory( m_vkLocalDevice, vkDeviceMemory );
        }

        // transfer from CPU to GPU memory
        {
            VkBufferMemoryBarrier memoryBarrier {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .dstbuffer = static_cast<VkBuffer>( allocation.dstbuffer ),
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
                    .aspectMask = ConvertAspect( dstTexture->vkBufferUsage ),
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

        // copy dstbuffer to texture
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
                    .aspectMask = ConvertAspect( dstTexture->vkBufferUsage ),
                    .mipLevel = mip,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                };
                imageCopy.imageOffset = {};
                imageCopy.imageExtent = { textureMip.width, textureMip.height, 1 };
            }

            vkCmdCopyBufferToImage(
                commandBuffer,
                static_cast<VkBuffer>( allocation.dstbuffer ),
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
                    .aspectMask = ConvertAspect( dstTexture->vkBufferUsage ),
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
        PerFrameData& frameData = getFrameData( commandQueue->frameCount );
        GraphicsBufferAllocation allocation = frameData.perFrameStagingBuffer.allocate( graphicsBufferUpdateDesc->size );

        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>( commandQueue->commandBuffer );

        // copy data to staging dstbuffer
        {
            VkDeviceMemory vkDeviceMemory = static_cast<VkDeviceMemory>( allocation.vkDeviceMemory );

            void* dstMemory {};
            HR( vkMapMemory( m_vkLocalDevice, vkDeviceMemory, allocation.offset, allocation.size, 0, &dstMemory ) );

            zp_memcpy( dstMemory, allocation.size, graphicsBufferUpdateDesc->data, graphicsBufferUpdateDesc->size );

            VkMappedMemoryRange flushMemoryRange {
                .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                .memory = static_cast<VkDeviceMemory>(allocation.vkDeviceMemory),
                .offset = allocation.offset,
                .size = allocation.size,
            };
            vkFlushMappedMemoryRanges( m_vkLocalDevice, 1, &flushMemoryRange );

            vkUnmapMemory( m_vkLocalDevice, vkDeviceMemory );
        }

        // transfer from CPU to GPU memory
        if( false )
        {
            VkBufferMemoryBarrier memoryBarrier[2] {
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                    .dstbuffer = static_cast<VkBuffer>( allocation.dstbuffer ),
                    .offset = allocation.offset,
                    .size = allocation.size,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dstbuffer = static_cast<VkBuffer>( dstGraphicsBuffer->dstbuffer ),
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

        // copy staging dstbuffer to dst dstbuffer
        {
            VkBufferCopy bufferCopy {
                .srcOffset = allocation.offset,
                .dstOffset = dstGraphicsBuffer->offset,
                .size = allocation.size,
            };

            vkCmdCopyBuffer(
                commandBuffer,
                static_cast<VkBuffer>( allocation.dstbuffer ),
                static_cast<VkBuffer>( dstGraphicsBuffer->dstbuffer ),
                1,
                &bufferCopy
            );
        }

        // finalize dstbuffer upload
        {
            VkBufferMemoryBarrier memoryBarrier {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstbuffer = static_cast<VkBuffer>( dstGraphicsBuffer->dstbuffer ),
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

    void VulkanGraphicsDevice::drawIndirect( const GraphicsBuffer& dstbuffer, zp_uint32_t drawCount, zp_uint32_t stride, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        VkBuffer indirectBuffer = static_cast<VkBuffer>(dstbuffer.dstbuffer);
        vkCmdDrawIndirect( commandBuffer, indirectBuffer, dstbuffer.offset, drawCount, stride );
    }

    void VulkanGraphicsDevice::drawIndexed( zp_uint32_t indexCount, zp_uint32_t instanceCount, zp_uint32_t firstIndex, zp_int32_t vertexOffset, zp_uint32_t firstInstance, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        vkCmdDrawIndexed( commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
    }

    void VulkanGraphicsDevice::drawIndexedIndirect( const GraphicsBuffer& dstbuffer, zp_uint32_t drawCount, zp_uint32_t stride, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        VkBuffer indirectBuffer = static_cast<VkBuffer>(dstbuffer.dstbuffer);
        vkCmdDrawIndexedIndirect( commandBuffer, indirectBuffer, dstbuffer.offset, drawCount, stride );
    }

    void VulkanGraphicsDevice::dispatch( zp_uint32_t groupCountX, zp_uint32_t groupCountY, zp_uint32_t groupCountZ, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        vkCmdDispatch( commandBuffer, groupCountX, groupCountY, groupCountZ );
    }

    void VulkanGraphicsDevice::dispatchIndirect( const GraphicsBuffer& dstbuffer, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        VkBuffer indirectBuffer = static_cast<VkBuffer>(dstbuffer.dstbuffer);
        vkCmdDispatchIndirect( commandBuffer, indirectBuffer, dstbuffer.offset );
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
                if( delayedDestroy.m_handle == nullptr )
                {
                    m_delayedDestroy.eraseAtSwapBack( i );
                    --i;
                }
                else if( m_currentFrameIndex < delayedDestroy.frameCount || ( m_currentFrameIndex - delayedDestroy.frameCount ) >= kMaxFrameDistance )
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

                // destroy each delayed destroy m_handle
                for( const DelayedDestroy& delayedDestroy : handlesToDestroy )
                {
                    DestroyDelayedDestroyHandle( delayedDestroy );
                }
            }
        }
    }

    void VulkanGraphicsDevice::destroyAllDelayedDestroy()
    {
        // destroy each delayed destroy m_handle
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

        // TODO: m_handle minimize properly
        // recreate new swapchain using old values
        createSwapchain( windowHandle, width, height, displayFormat, colorSpace );
    }

    VulkanGraphicsDevice::PerFrameData& VulkanGraphicsDevice::getCurrentFrameData()
    {
        const zp_uint64_t frame = m_currentFrameIndex & ( kMaxBufferedFrameCount - 1 );
        return m_perFrameData[ frame ];
    }

    const VulkanGraphicsDevice::PerFrameData& VulkanGraphicsDevice::getCurrentFrameData() const
    {
        const zp_uint64_t frame = m_currentFrameIndex & ( kMaxBufferedFrameCount - 1 );
        return m_perFrameData[ frame ];
    }

    VulkanGraphicsDevice::PerFrameData& VulkanGraphicsDevice::getFrameData( zp_uint64_t frameCount )
    {
        const zp_uint64_t frame = frameCount & ( kMaxBufferedFrameCount - 1 );
        return m_perFrameData[ frame ];
    }
#endif
}
