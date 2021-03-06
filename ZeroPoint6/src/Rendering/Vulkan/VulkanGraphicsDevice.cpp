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

#include "Engine/MemoryLabels.h"

#include "Rendering/GraphicsDevice.h"
#include "Rendering/Vulkan/VulkanGraphicsDevice.h"

#include <vulkan/vulkan.h>
#include <windows.h>

#if ZP_OS_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN

#include <vulkan/vulkan_win32.h>

#endif

#if ZP_DEBUG
#define HR( r )   do { const VkResult ZP_CONCAT(_result_,__LINE__) = (r); if( ZP_CONCAT(_result_,__LINE__) != VK_SUCCESS ) { zp_printfln("HR Failed: " #r ); zp_debug_break(); } } while( false )
#else
#define HR( r )   r
#endif

namespace zp
{
    namespace
    {
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

        VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
        {
            const zp_uint32_t messageMask = 0
                                            #if ZP_DEBUG
                                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                            #endif
                                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

            if( messageSeverity & messageMask )
            {
                char buff[1024];
                zp_snprintf( buff, pCallbackData->pMessage );

                zp_int32_t result = GetPlatform()->ShowMessageBox( nullptr, "Vulkan Error", buff );
                if( result == 0 )
                {
                    exit( 1 );
                }
                else if( result == 1 )
                {
                    zp_debug_break();
                }

                //zp_printfln( "validation layer: %s\n", pCallbackData->pMessage );
                //zp_debug_break();
            }

            const VkBool32 shouldAbort = messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? VK_TRUE : VK_FALSE;
            return shouldAbort;
        }

#endif // ZP_DEBUG

#if ZP_DEBUG
        const char* kValidationLayers[] = {
            "VK_LAYER_KHRONOS_validation"
        };
#endif

        void FillLayerNames( const char* const*& ppEnabledLayerNames, uint32_t& enabledLayerCount )
        {
#if ZP_DEBUG
            ppEnabledLayerNames = kValidationLayers;
            enabledLayerCount = ZP_ARRAY_SIZE( kValidationLayers );
#else
            ppEnabledLayerNames = nullptr;
            enabledLayerCount = 0;
#endif
        }

        zp_bool_t IsPhysicalDeviceSuitable( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, GraphicsDeviceFeatures graphicsDeviceFeatures )
        {
            zp_bool_t isSuitable = true;

            VkPhysicalDeviceProperties physicalDeviceProperties;
            VkPhysicalDeviceFeatures physicalDeviceFeatures;
            vkGetPhysicalDeviceProperties( physicalDevice, &physicalDeviceProperties );
            vkGetPhysicalDeviceFeatures( physicalDevice, &physicalDeviceFeatures );

            // require discrete gpu
            isSuitable &= physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

            // support geometry shaders
            if( graphicsDeviceFeatures & GraphicsDeviceFeatures::GeometryShaderSupport )
            {
                isSuitable &= physicalDeviceFeatures.geometryShader == VK_TRUE;
            }

            // support tessellation shaders
            if( graphicsDeviceFeatures & GraphicsDeviceFeatures::TessellationShaderSupport )
            {
                isSuitable &= physicalDeviceFeatures.tessellationShader == VK_TRUE;
            }

#if ZP_USE_PROFILER
            isSuitable &= physicalDeviceProperties.limits.timestampComputeAndGraphics == VK_TRUE;

            isSuitable &= physicalDeviceFeatures.pipelineStatisticsQuery == VK_TRUE;
#endif

            const char* requiredExtensions[] {
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
                for( auto requiredExtension : requiredExtensions )
                {
                    if( zp_strcmp( availableExtension.extensionName, requiredExtension ) == 0 )
                    {
                        ++foundRequiredExtensions;
                        break;
                    }
                }
            }

            // check that required extensions are available
            const zp_bool_t areExtensionsAvailable = foundRequiredExtensions == ZP_ARRAY_SIZE( requiredExtensions );
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

        VkSurfaceFormatKHR ChooseSwapChainSurfaceFormat( const Vector<VkSurfaceFormatKHR>& surfaceFormats, int format, int colorSpace )
        {
            VkSurfaceFormatKHR chosenFormat = surfaceFormats[ 0 ];

            for( const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats )
            {
                // VK_FORMAT_B8G8R8A8_SNORM
                // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                if( surfaceFormat.format == format && surfaceFormat.colorSpace == colorSpace )
                {
                    chosenFormat = surfaceFormat;
                    break;
                }
            }

            return chosenFormat;
        }

        VkPresentModeKHR ChooseSwapChainPresentMode( const Vector<VkPresentModeKHR>& presentModes, zp_bool_t vsync )
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

        VkExtent2D ChooseSwapChainExtent( const VkSurfaceCapabilitiesKHR& surfaceCapabilities, uint32_t requestedWith, uint32_t requestedHeight )
        {
            VkExtent2D chosenExtents;

            if( surfaceCapabilities.currentExtent.width != UINT32_MAX )
            {
                chosenExtents = surfaceCapabilities.currentExtent;
            }
            else
            {
                VkExtent2D actualExtents = { requestedWith, requestedHeight };

                actualExtents.width = zp_clamp(
                    actualExtents.width,
                    surfaceCapabilities.minImageExtent.width,
                    surfaceCapabilities.maxImageExtent.width );
                actualExtents.height = zp_clamp(
                    actualExtents.height,
                    surfaceCapabilities.minImageExtent.height,
                    surfaceCapabilities.maxImageExtent.height );

                chosenExtents = actualExtents;
            }

            return chosenExtents;

        }

        constexpr uint32_t FindMemoryTypeIndex( const VkPhysicalDeviceMemoryProperties* physicalDeviceMemoryProperties, const zp_uint32_t typeFilter, const VkMemoryPropertyFlags memoryPropertyFlags )
        {
            for( uint32_t i = 0; i < physicalDeviceMemoryProperties->memoryTypeCount; i++ )
            {
                if( ( typeFilter & ( 1 << i ) ) && ( physicalDeviceMemoryProperties->memoryTypes[ i ].propertyFlags & memoryPropertyFlags ) == memoryPropertyFlags )
                {
                    return i;
                }
            }

            ZP_INVALID_CODE_PATH();
            return 0;
        }

        constexpr VkIndexType Convert( const IndexBufferFormat indexBufferFormat )
        {
            constexpr VkIndexType indexTypeMap[] {
                VK_INDEX_TYPE_UINT16,
                VK_INDEX_TYPE_UINT32,
                VK_INDEX_TYPE_UINT8_EXT,
                VK_INDEX_TYPE_NONE_KHR,
            };
            return indexTypeMap[ indexBufferFormat ];
        }

        constexpr VkBufferUsageFlags Convert( const GraphicsBufferUsageFlags graphicsBufferUsageFlags )
        {
            VkBufferUsageFlags bufferUsageFlagBits {};

            if( graphicsBufferUsageFlags & ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_SRC )
            {
                bufferUsageFlagBits |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            }

            if( graphicsBufferUsageFlags & ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_DEST )
            {
                bufferUsageFlagBits |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            }

            if( graphicsBufferUsageFlags & ZP_GRAPHICS_BUFFER_USAGE_VERTEX_BUFFER )
            {
                bufferUsageFlagBits |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }

            if( graphicsBufferUsageFlags & ZP_GRAPHICS_BUFFER_USAGE_INDEX_BUFFER )
            {
                bufferUsageFlagBits |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }

            if( graphicsBufferUsageFlags & ZP_GRAPHICS_BUFFER_USAGE_INDIRECT_ARGS )
            {
                bufferUsageFlagBits |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }

            if( graphicsBufferUsageFlags & ZP_GRAPHICS_BUFFER_USAGE_UNIFORM )
            {
                bufferUsageFlagBits |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }

            if( graphicsBufferUsageFlags & ZP_GRAPHICS_BUFFER_USAGE_UNIFORM_TEXEL )
            {
                bufferUsageFlagBits |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            }

            if( graphicsBufferUsageFlags & ZP_GRAPHICS_BUFFER_USAGE_STORAGE )
            {
                bufferUsageFlagBits |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }

            if( graphicsBufferUsageFlags & ZP_GRAPHICS_BUFFER_USAGE_STORAGE_TEXEL )
            {
                bufferUsageFlagBits |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
            }

            return bufferUsageFlagBits;
        }

        constexpr VkMemoryPropertyFlags Convert( MemoryPropertyFlags memoryPropertyFlags )
        {
            VkMemoryPropertyFlags propertyFlags {};

            if( memoryPropertyFlags & ZP_MEMORY_PROPERTY_DEVICE_LOCAL )
            {
                propertyFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            }

            if( memoryPropertyFlags & ZP_MEMORY_PROPERTY_HOST_VISIBLE )
            {
                propertyFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            }

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

            return primitiveTopologyMap[ topology ];
        }

        constexpr VkPolygonMode Convert( PolygonFillMode polygonFillMode )
        {
            constexpr VkPolygonMode polygonModeMap[] {
                VK_POLYGON_MODE_FILL,
                VK_POLYGON_MODE_LINE,
                VK_POLYGON_MODE_POINT,
            };

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

            return cullModeFlagsMap[ cullMode ];
        }

        constexpr VkFrontFace Convert( FrontFaceMode frontFaceMode )
        {
            constexpr VkFrontFace frontFaceMap[] {
                VK_FRONT_FACE_COUNTER_CLOCKWISE,
                VK_FRONT_FACE_CLOCKWISE,
            };

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
                VK_LOGIC_OP_SET,
            };

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

            return blendFactorMap[ blendFactor ];
        }

        constexpr VkColorComponentFlags Convert( ColorComponent colorComponent )
        {
            VkColorComponentFlags colorComponentFlags {};

            colorComponentFlags |= colorComponent & ZP_COLOR_COMPONENT_R ? VK_COLOR_COMPONENT_R_BIT : 0;
            colorComponentFlags |= colorComponent & ZP_COLOR_COMPONENT_G ? VK_COLOR_COMPONENT_G_BIT : 0;
            colorComponentFlags |= colorComponent & ZP_COLOR_COMPONENT_B ? VK_COLOR_COMPONENT_B_BIT : 0;
            colorComponentFlags |= colorComponent & ZP_COLOR_COMPONENT_A ? VK_COLOR_COMPONENT_A_BIT : 0;

            return colorComponentFlags;
        }

        constexpr VkVertexInputRate Convert( VertexInputRate vertexInputRate )
        {
            constexpr VkVertexInputRate vertexInputRateMap[] {
                VK_VERTEX_INPUT_RATE_VERTEX,
                VK_VERTEX_INPUT_RATE_INSTANCE
            };

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
                VK_SHADER_STAGE_ALL_GRAPHICS,
                VK_SHADER_STAGE_ALL
            };

            return shaderStageMap[ shaderStage ];
        }

        constexpr VkPipelineBindPoint Convert( PipelineBindPoint bindPoint )
        {
            constexpr VkPipelineBindPoint bindPointMap[] {
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                VK_PIPELINE_BIND_POINT_COMPUTE,
            };

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

            return imageTypeMap[ textureDimension ];
        }

        constexpr VkFormat Convert( TextureFormat textureFormat )
        {
            constexpr VkFormat formatMap[] {
                VK_FORMAT_UNDEFINED,
            };

            ZP_ASSERT( static_cast<zp_size_t>( textureFormat ) < ZP_ARRAY_SIZE( formatMap ) );
            return formatMap[ textureFormat ];
        }

        constexpr VkImageUsageFlags Convert( TextureUsage usage )
        {
            VkImageUsageFlags imageUsage = 0;

            if( usage & ZP_TEXTURE_USAGE_TRANSFER_SRC )
            {
                imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }

            if( usage & ZP_TEXTURE_USAGE_TRANSFER_DST )
            {
                imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }

            if( usage & ZP_TEXTURE_USAGE_SAMPLED )
            {
                imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            }

            if( usage & ZP_TEXTURE_USAGE_STORAGE )
            {
                imageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
            }

            if( usage & ZP_TEXTURE_USAGE_COLOR_ATTACHMENT )
            {
                imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }

            if( usage & ZP_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT )
            {
                imageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }

            if( usage & ZP_TEXTURE_USAGE_TRANSIENT_ATTACHMENT )
            {
                imageUsage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
            }

            return imageUsage;
        }

        constexpr VkImageAspectFlags ConvertAspect( TextureUsage textureUsage )
        {
            VkImageAspectFlags aspectFlags = 0;

            if( textureUsage & ZP_TEXTURE_USAGE_COLOR_ATTACHMENT )
            {
                aspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
            }

            if( textureUsage & ZP_TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT )
            {
                aspectFlags |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }

            return aspectFlags;
        }

        constexpr VkFilter Convert( FilterMode filterMode )
        {
            constexpr VkFilter filterMap[] {
                VK_FILTER_NEAREST,
                VK_FILTER_LINEAR,
            };

            return filterMap[ filterMode ];
        }

        constexpr VkSamplerMipmapMode Convert( MipmapMode mipmapMode )
        {
            constexpr VkSamplerMipmapMode mipmapModeMap[] {
                VK_SAMPLER_MIPMAP_MODE_NEAREST,
                VK_SAMPLER_MIPMAP_MODE_LINEAR,
            };

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

            return borderColorMap[ borderColor ];
        }
    }

    //
    //
    //

    VulkanGraphicsDevice::VulkanGraphicsDevice( MemoryLabel memoryLabel, GraphicsDeviceFeatures graphicsDeviceFeatures )
        : GraphicsDevice( memoryLabel )
        , m_perFrameData {}
        , m_vkInstance( VK_NULL_HANDLE )
        , m_vkSurface( VK_NULL_HANDLE )
        , m_vkPhysicalDevice( VK_NULL_HANDLE )
        , m_vkLocalDevice( VK_NULL_HANDLE )
        , m_vkRenderQueues {}
        , m_vkSwapChain( VK_NULL_HANDLE )
        , m_vkSwapChainRenderPass( VK_NULL_HANDLE )
        , m_vkPipelineCache( VK_NULL_HANDLE )
        , m_vkGraphicsCommandPool( VK_NULL_HANDLE )
        , m_vkTransferCommandPool( VK_NULL_HANDLE )
        , m_vkComputeCommandPool( VK_NULL_HANDLE )
        , m_vkSwapChainFormat( VK_FORMAT_UNDEFINED )
        , m_vkSwapChainColorSpace( VK_COLORSPACE_SRGB_NONLINEAR_KHR )
        , m_vkSwapChainExtent { 0, 0 }
#if ZP_DEBUG
        , m_vkDebugMessenger( VK_NULL_HANDLE )
#endif
        , m_swapChainImages( 4, memoryLabel )
        , m_swapChainImageViews( 4, memoryLabel )
        , m_swapChainFrameBuffers( 4, memoryLabel )
        , m_swapChainInFlightFences( 4, memoryLabel )
        , m_stagingBuffer {}
        , m_queueFamilies { VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED }
    //, m_commandQueueCount( 0 )
    //, m_commandQueues( 4, memoryLabel )
    {
        zp_zero_memory_array( m_perFrameData );

        VkApplicationInfo applicationInfo {};
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pApplicationName = "ZeroPoint 6 Application";
        applicationInfo.apiVersion = VK_MAKE_VERSION( 1, 0, 0 );
        applicationInfo.pEngineName = "ZeroPoint 6";
        applicationInfo.engineVersion = VK_MAKE_VERSION( ZP_VERSION_MAJOR, ZP_VERSION_MINOR, ZP_VERSION_PATCH );
        applicationInfo.apiVersion = VK_API_VERSION_1_2;

        const char* extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
#if ZP_OS_WINDOWS
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if ZP_DEBUG
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        };

        // create instance
        VkInstanceCreateInfo instanceCreateInfo {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &applicationInfo;
        instanceCreateInfo.enabledExtensionCount = ZP_ARRAY_SIZE( extensions );
        instanceCreateInfo.ppEnabledExtensionNames = extensions;

        FillLayerNames( instanceCreateInfo.ppEnabledLayerNames, instanceCreateInfo.enabledLayerCount );

#if ZP_DEBUG
        // add debug info to create instance
        VkDebugUtilsMessengerCreateInfoEXT createInstanceDebugMessengerInfo {};
        createInstanceDebugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInstanceDebugMessengerInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInstanceDebugMessengerInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInstanceDebugMessengerInfo.pfnUserCallback = DebugCallback;
        createInstanceDebugMessengerInfo.pUserData = nullptr; // Optional

        instanceCreateInfo.pNext = &createInstanceDebugMessengerInfo;
#endif
        HR( vkCreateInstance( &instanceCreateInfo, nullptr, &m_vkInstance ) );

#if ZP_DEBUG
        // create debug messenger
        VkDebugUtilsMessengerCreateInfoEXT createDebugMessengerInfo = {};
        createDebugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createDebugMessengerInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createDebugMessengerInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createDebugMessengerInfo.pfnUserCallback = DebugCallback;
        createDebugMessengerInfo.pUserData = nullptr; // Optional

        HR( CallDebugUtilResult( vkCreateDebugUtilsMessengerEXT, m_vkInstance, m_vkInstance, &createDebugMessengerInfo, nullptr, &m_vkDebugMessenger ) );
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
                if( IsPhysicalDeviceSuitable( physicalDevice, m_vkSurface, graphicsDeviceFeatures ) )
                {
                    m_vkPhysicalDevice = physicalDevice;
                    break;
                }
            }
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

                if( m_queueFamilies.graphicsFamily == VK_QUEUE_FAMILY_IGNORED && queueFamilyProperty.queueCount > 0 && queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT )
                {
                    m_queueFamilies.graphicsFamily = i;

                    VkBool32 presentSupport = VK_TRUE;// VK_FALSE;
                    //HR( vkGetPhysicalDeviceSurfaceSupportKHR( m_vkPhysicalDevice, i, m_vkSurface, &presentSupport ));

                    if( m_queueFamilies.presentFamily == VK_QUEUE_FAMILY_IGNORED && presentSupport )
                    {
                        m_queueFamilies.presentFamily = i;
                    }
                }

                if( m_queueFamilies.transferFamily == VK_QUEUE_FAMILY_IGNORED && queueFamilyProperty.queueCount > 0 && queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT )
                {
                    m_queueFamilies.transferFamily = i;
                }

                if( m_queueFamilies.computeFamily == VK_QUEUE_FAMILY_IGNORED && queueFamilyProperty.queueCount > 0 && queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT )
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
                VkDeviceQueueCreateInfo queueCreateInfo {};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;

                deviceQueueCreateInfos.pushBack( queueCreateInfo );
            }

            const char* deviceExtensions[] {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };

            VkPhysicalDeviceFeatures deviceFeatures {};
            vkGetPhysicalDeviceFeatures( m_vkPhysicalDevice, &deviceFeatures );

            VkDeviceCreateInfo localDeviceCreateInfo {};
            localDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            localDeviceCreateInfo.queueCreateInfoCount = deviceQueueCreateInfos.size();
            localDeviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
            localDeviceCreateInfo.pEnabledFeatures = &deviceFeatures;
            localDeviceCreateInfo.enabledExtensionCount = ZP_ARRAY_SIZE( deviceExtensions );
            localDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

            FillLayerNames( localDeviceCreateInfo.ppEnabledLayerNames, localDeviceCreateInfo.enabledLayerCount );

            HR( vkCreateDevice( m_vkPhysicalDevice, &localDeviceCreateInfo, nullptr, &m_vkLocalDevice ) );

            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.graphicsFamily, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_GRAPHICS ] );
            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.transferFamily, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_TRANSFER ] );
            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.computeFamily, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_COMPUTE ] );
            vkGetDeviceQueue( m_vkLocalDevice, m_queueFamilies.presentFamily, 0, &m_vkRenderQueues[ ZP_RENDER_QUEUE_PRESENT ] );
        }

        // create pipeline cache
        {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo {};
            pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

            HR( vkCreatePipelineCache( m_vkLocalDevice, &pipelineCacheCreateInfo, nullptr, &m_vkPipelineCache ) );
        }

        // create command pools
        {
            VkCommandPoolCreateInfo commandPoolCreateInfo {};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.graphicsFamily;
            HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, nullptr, &m_vkGraphicsCommandPool ) );

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.transferFamily;
            HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, nullptr, &m_vkTransferCommandPool ) );

            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolCreateInfo.queueFamilyIndex = m_queueFamilies.computeFamily;
            HR( vkCreateCommandPool( m_vkLocalDevice, &commandPoolCreateInfo, nullptr, &m_vkComputeCommandPool ) );
        }

        // create staging buffer
        {
            const zp_size_t stagingBufferSize = 32 MB;
            const zp_size_t perFrameStagingBufferSize = stagingBufferSize / kBufferedFrameCount;

            GraphicsBufferDesc graphicsBufferDesc {};
            graphicsBufferDesc.name = "Staging Buffer";
            graphicsBufferDesc.size = stagingBufferSize;
            graphicsBufferDesc.graphicsBufferUsageFlags = ZP_GRAPHICS_BUFFER_USAGE_TRANSFER_SRC | ZP_GRAPHICS_BUFFER_USAGE_STORAGE;
            graphicsBufferDesc.memoryPropertyFlags = ZP_MEMORY_PROPERTY_HOST_VISIBLE;

            createBuffer( &graphicsBufferDesc, &m_stagingBuffer );
        }
    }

    VulkanGraphicsDevice::~VulkanGraphicsDevice()
    {
        HR( vkDeviceWaitIdle( m_vkLocalDevice ) );

        destroySwapChain();

        destroyBuffer( &m_stagingBuffer );

        vkDestroyCommandPool( m_vkLocalDevice, m_vkGraphicsCommandPool, nullptr );
        vkDestroyCommandPool( m_vkLocalDevice, m_vkTransferCommandPool, nullptr );
        vkDestroyCommandPool( m_vkLocalDevice, m_vkComputeCommandPool, nullptr );
        m_vkGraphicsCommandPool = {};
        m_vkTransferCommandPool = {};
        m_vkComputeCommandPool = {};

        vkDestroyPipelineCache( m_vkLocalDevice, m_vkPipelineCache, nullptr );
        m_vkPipelineCache = {};

        vkDestroyDevice( m_vkLocalDevice, nullptr );
        m_vkLocalDevice = {};
        m_vkPhysicalDevice = {};
        zp_zero_memory_array( m_vkRenderQueues );

        vkDestroySurfaceKHR( m_vkInstance, m_vkSurface, nullptr );
        m_vkSurface = {};

#if ZP_DEBUG
        CallDebugUtil( vkDestroyDebugUtilsMessengerEXT, m_vkInstance, m_vkInstance, m_vkDebugMessenger, nullptr );
        m_vkDebugMessenger = {};
#endif

        vkDestroyInstance( m_vkInstance, nullptr );
        m_vkInstance = {};
    }

    void VulkanGraphicsDevice::createSwapChain( zp_handle_t windowHandle, zp_uint32_t width, zp_uint32_t height, int displayFormat, int colorSpace )
    {
#if ZP_OS_WINDOWS
        auto hWnd = static_cast<HWND>( windowHandle );
        auto hInstance = reinterpret_cast<HINSTANCE>(::GetWindowLongPtr( hWnd, GWLP_HINSTANCE ));

        // create surface
        VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
        win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        win32SurfaceCreateInfo.hwnd = hWnd;
        win32SurfaceCreateInfo.hinstance = hInstance;

        HR( vkCreateWin32SurfaceKHR( m_vkInstance, &win32SurfaceCreateInfo, nullptr, &m_vkSurface ) );

        VkBool32 surfaceSupported;
        HR( vkGetPhysicalDeviceSurfaceSupportKHR( m_vkPhysicalDevice, m_queueFamilies.presentFamily, m_vkSurface, &surfaceSupported ) );
        ZP_ASSERT( surfaceSupported );
#endif

        VkSurfaceCapabilitiesKHR swapChainSupportDetails;
        HR( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_vkPhysicalDevice, m_vkSurface, &swapChainSupportDetails ) );

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
        colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapChainSurfaceFormat( surfaceFormats, displayFormat, colorSpace );
        VkPresentModeKHR presentMode = ChooseSwapChainPresentMode( presentModes, false );
        VkExtent2D extent = ChooseSwapChainExtent( swapChainSupportDetails, swapChainSupportDetails.currentExtent.width, swapChainSupportDetails.currentExtent.height );

        uint32_t imageCount = swapChainSupportDetails.minImageCount + 1;
        if( swapChainSupportDetails.maxImageCount > 0 )
        {
            imageCount = imageCount < swapChainSupportDetails.maxImageCount ? imageCount : swapChainSupportDetails.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapChainCreateInfo {};
        swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCreateInfo.surface = m_vkSurface;
        swapChainCreateInfo.minImageCount = imageCount;
        swapChainCreateInfo.imageFormat = surfaceFormat.format;
        swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapChainCreateInfo.imageExtent = extent;
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

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

        swapChainCreateInfo.preTransform = swapChainSupportDetails.currentTransform;
        swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapChainCreateInfo.presentMode = presentMode;
        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = m_vkSwapChain;

        HR( vkCreateSwapchainKHR( m_vkLocalDevice, &swapChainCreateInfo, nullptr, &m_vkSwapChain ) );

        if( swapChainCreateInfo.oldSwapchain != VK_NULL_HANDLE )
        {
            vkDestroySwapchainKHR( m_vkLocalDevice, swapChainCreateInfo.oldSwapchain, nullptr );
        }

        m_vkSwapChainFormat = surfaceFormat.format;
        m_vkSwapChainColorSpace = (VkColorSpaceKHR)colorSpace;
        m_vkSwapChainExtent = extent;

        uint32_t swapChainImageCount = 0;
        HR( vkGetSwapchainImagesKHR( m_vkLocalDevice, m_vkSwapChain, &swapChainImageCount, VK_NULL_HANDLE ) );

        m_swapChainInFlightFences.reserve( swapChainImageCount );
        m_swapChainInFlightFences.resize_unsafe( swapChainImageCount );

        m_swapChainImages.reserve( swapChainImageCount );
        m_swapChainImages.resize_unsafe( swapChainImageCount );
        HR( vkGetSwapchainImagesKHR( m_vkLocalDevice, m_vkSwapChain, &swapChainImageCount, m_swapChainImages.data() ) );

        m_swapChainImageViews.reserve( swapChainImageCount );
        m_swapChainImageViews.resize_unsafe( swapChainImageCount );

        {
            if( m_vkSwapChainRenderPass )
            {
                vkDestroyRenderPass( m_vkLocalDevice, m_vkSwapChainRenderPass, nullptr );
            }

            VkAttachmentDescription colorAttachment {};
            colorAttachment.format = m_vkSwapChainFormat;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachmentRef {};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subPassDesc {};
            subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subPassDesc.colorAttachmentCount = 1;
            subPassDesc.pColorAttachments = &colorAttachmentRef;

            VkSubpassDependency subPassDependency {};
            subPassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            subPassDependency.dstSubpass = 0;
            subPassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subPassDependency.srcAccessMask = 0;
            subPassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subPassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo renderPassInfo {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &colorAttachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subPassDesc;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &subPassDependency;

            HR( vkCreateRenderPass( m_vkLocalDevice, &renderPassInfo, nullptr, &m_vkSwapChainRenderPass ) );
        }

        for( zp_size_t i = 0; i < swapChainImageCount; ++i )
        {
            VkImageViewCreateInfo imageViewCreateInfo {};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = m_swapChainImages[ i ];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = m_vkSwapChainFormat;
            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;

            if( m_swapChainImageViews[ i ] != VK_NULL_HANDLE )
            {
                // TODO: destroy in a few frames
                vkDestroyImageView( m_vkLocalDevice, m_swapChainImageViews[ i ], nullptr );
                m_swapChainImageViews[ i ] = VK_NULL_HANDLE;
            }

            HR( vkCreateImageView( m_vkLocalDevice, &imageViewCreateInfo, nullptr, &m_swapChainImageViews[ i ] ) );

            VkImageView attachments[] = { m_swapChainImageViews[ i ] };

            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = m_vkSwapChainRenderPass;
            framebufferCreateInfo.attachmentCount = ZP_ARRAY_SIZE( attachments );
            framebufferCreateInfo.pAttachments = attachments;
            framebufferCreateInfo.width = m_vkSwapChainExtent.width;
            framebufferCreateInfo.height = m_vkSwapChainExtent.height;
            framebufferCreateInfo.layers = 1;

            if( m_swapChainFrameBuffers[ i ] != VK_NULL_HANDLE )
            {
                // TODO: destroy in a few frames
                vkDestroyFramebuffer( m_vkLocalDevice, m_swapChainFrameBuffers[ i ], nullptr );
                m_swapChainFrameBuffers[ i ] = VK_NULL_HANDLE;
            }

            HR( vkCreateFramebuffer( m_vkLocalDevice, &framebufferCreateInfo, nullptr, &m_swapChainFrameBuffers[ i ] ) );
        }

        // swap chain sync
        VkSemaphoreCreateInfo semaphoreCreateInfo {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

#if ZP_USE_PROFILER
        VkQueryPoolCreateInfo pipelineStatsQueryPoolCreateInfo {};
        pipelineStatsQueryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        pipelineStatsQueryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
        pipelineStatsQueryPoolCreateInfo.pipelineStatistics =
            VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
        pipelineStatsQueryPoolCreateInfo.queryCount = 8;

        VkQueryPoolCreateInfo timestampQueryPoolCreateInfo {};
        timestampQueryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        timestampQueryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        timestampQueryPoolCreateInfo.queryCount = 16;
#endif

        zp_size_t perFrameStagingBufferOffset = 0;
        const zp_size_t perFrameStagingBufferSize = m_stagingBuffer.size / kBufferedFrameCount;

        for( PerFrameData& perFrameData : m_perFrameData )
        {
            HR( vkCreateSemaphore( m_vkLocalDevice, &semaphoreCreateInfo, nullptr, &perFrameData.vkSwapChainAcquireSemaphore ) );
            HR( vkCreateSemaphore( m_vkLocalDevice, &semaphoreCreateInfo, nullptr, &perFrameData.vkRenderFinishedSemaphore ) );
            HR( vkCreateFence( m_vkLocalDevice, &fenceCreateInfo, nullptr, &perFrameData.vkInFlightFences ) );
            HR( vkCreateFence( m_vkLocalDevice, &fenceCreateInfo, nullptr, &perFrameData.vkSwapChainImageAcquiredFence ) );
#if ZP_USE_PROFILER
            HR( vkCreateQueryPool( m_vkLocalDevice, &pipelineStatsQueryPoolCreateInfo, nullptr, &perFrameData.vkPipelineStatisticsQueryPool ) );
            HR( vkCreateQueryPool( m_vkLocalDevice, &timestampQueryPoolCreateInfo, nullptr, &perFrameData.vkTimestampQueryPool ) );
#endif
            perFrameData.perFrameStagingBuffer.graphicsBuffer = m_stagingBuffer.splitBuffer( perFrameStagingBufferOffset, perFrameStagingBufferSize );
            perFrameData.perFrameStagingBuffer.allocated = 0;

            perFrameData.commandQueueCount = 0;
            perFrameData.commandQueueCapacity = 0;
            perFrameData.commandQueues = nullptr;

            perFrameStagingBufferOffset += perFrameStagingBufferSize;
        }
    }

    void VulkanGraphicsDevice::destroySwapChain()
    {
        const VkCommandPool commandPoolMap[] {
            m_vkGraphicsCommandPool,
            m_vkTransferCommandPool,
            m_vkComputeCommandPool,
            m_vkGraphicsCommandPool
        };

        for( PerFrameData& perFrameData : m_perFrameData )
        {
            vkDestroySemaphore( m_vkLocalDevice, perFrameData.vkSwapChainAcquireSemaphore, nullptr );
            vkDestroySemaphore( m_vkLocalDevice, perFrameData.vkRenderFinishedSemaphore, nullptr );
            vkDestroyFence( m_vkLocalDevice, perFrameData.vkInFlightFences, nullptr );
            vkDestroyFence( m_vkLocalDevice, perFrameData.vkSwapChainImageAcquiredFence, nullptr );

#if ZP_USE_PROFILER
            vkDestroyQueryPool( m_vkLocalDevice, perFrameData.vkPipelineStatisticsQueryPool, nullptr );
            vkDestroyQueryPool( m_vkLocalDevice, perFrameData.vkTimestampQueryPool, nullptr );
#endif
            if( perFrameData.commandQueues )
            {
                for( zp_size_t i = 0; i < perFrameData.commandQueueCapacity; ++i )
                {
                    if( perFrameData.commandQueues && perFrameData.commandQueues[ i ].commandBuffer )
                    {
                        const auto commandBuffer = static_cast<VkCommandBuffer>( perFrameData.commandQueues[ i ].commandBuffer );
                        vkFreeCommandBuffers( m_vkLocalDevice, commandPoolMap[ perFrameData.commandQueues[ i ].queue ], 1, &commandBuffer );
                    }
                }

                GetAllocator( memoryLabel )->free( perFrameData.commandQueues );
            }

            perFrameData.vkSwapChainAcquireSemaphore = VK_NULL_HANDLE;
            perFrameData.vkRenderFinishedSemaphore = VK_NULL_HANDLE;
            perFrameData.vkInFlightFences = {};
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

        for( VkFramebuffer swapChainFramebuffer : m_swapChainFrameBuffers )
        {
            vkDestroyFramebuffer( m_vkLocalDevice, swapChainFramebuffer, nullptr );
        }
        m_swapChainFrameBuffers.clear();

        vkDestroyRenderPass( m_vkLocalDevice, m_vkSwapChainRenderPass, nullptr );
        m_vkSwapChainRenderPass = {};

        for( VkImageView swapChainImageView : m_swapChainImageViews )
        {
            vkDestroyImageView( m_vkLocalDevice, swapChainImageView, nullptr );
        }
        m_swapChainImageViews.clear();
        m_swapChainImages.clear();

        for( VkFence& inFlightFence : m_swapChainInFlightFences )
        {
            vkDestroyFence( m_vkLocalDevice, inFlightFence, nullptr );
        }
        m_swapChainInFlightFences.clear();

        vkDestroySwapchainKHR( m_vkLocalDevice, m_vkSwapChain, nullptr );
        m_vkSwapChain = {};
    }

    void VulkanGraphicsDevice::beginFrame( zp_uint64_t frameIndex )
    {
        ZP_PROFILE_CPU_BLOCK();

        VkResult result;

        const zp_uint64_t frame = frameIndex & ( kBufferedFrameCount - 1 );
        PerFrameData& frameData = m_perFrameData[ frame ];

        uint32_t imageIndex;
        result = vkAcquireNextImageKHR( m_vkLocalDevice, m_vkSwapChain, UINT64_MAX, frameData.vkSwapChainAcquireSemaphore, VK_NULL_HANDLE, &imageIndex );

        if( result != VK_SUCCESS )
        {
            if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR )
            {
                // TODO: rebuild swap chain
                beginFrame( frameIndex );
                return;
            }

            ZP_INVALID_CODE_PATH();
        }

        frameData.swapChainImageIndex = imageIndex;

        if( m_swapChainInFlightFences[ imageIndex ] != VK_NULL_HANDLE )
        {
            HR( vkWaitForFences( m_vkLocalDevice, 1, &m_swapChainInFlightFences[ imageIndex ], VK_TRUE, UINT64_MAX ) );
        }

        m_swapChainInFlightFences[ imageIndex ] = frameData.vkInFlightFences;

        for( zp_size_t i = 0; i < frameData.commandQueueCount; ++i )
        {
            HR( vkResetCommandBuffer( static_cast<VkCommandBuffer>( frameData.commandQueues[ i ].commandBuffer ), 0 ) );
        }

#if ZP_USE_PROFILER
        if( frameIndex > 0 )
        {
            const zp_uint64_t prevFrame = ( frameIndex - 1 ) & ( kBufferedFrameCount - 1 );
            PerFrameData& prevFrameData = m_perFrameData[ prevFrame ];

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

        frameData.perFrameStagingBuffer.allocated = 0;
        frameData.commandQueueCount = 0;
    }

    void VulkanGraphicsDevice::submit( zp_uint64_t frameIndex )
    {
        ZP_PROFILE_CPU_BLOCK();

        const zp_uint64_t frame = frameIndex & ( kBufferedFrameCount - 1 );
        PerFrameData& frameData = m_perFrameData[ frame ];

        const zp_size_t commandQueueCount = frameData.commandQueueCount;
        if( commandQueueCount > 0 )
        {
            const zp_size_t preCommandQueueIndex = commandQueueCount - 1;
            VkCommandBuffer buffer = static_cast<VkCommandBuffer>( frameData.commandQueues[ preCommandQueueIndex ].commandBuffer );

#if ZP_USE_PROFILER
            vkCmdWriteTimestamp( buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameData.vkTimestampQueryPool, preCommandQueueIndex * 2 + 1 );
            //vkCmdEndQuery( buffer, frameData.vkPipelineStatisticsQueryPool, 0 );
            //vkCmdResetQueryPool( static_cast<VkCommandBuffer>( frameData.commandQueues[ preCommandQueueIndex ].commandBuffer ), frameData.vkTimestampQueryPool, commandQueueCount * 2 + 2, 16 - (commandQueueCount * 2 + 2));
#endif
            HR( vkEndCommandBuffer( buffer ) );
        }

        VkPipelineStageFlags waitStages[] { VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT };

        VkSemaphore waitSemaphores[] { frameData.vkSwapChainAcquireSemaphore };
        ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( waitStages ) == ZP_ARRAY_SIZE( waitSemaphores ) );

        VkSemaphore signalSemaphores[] { frameData.vkRenderFinishedSemaphore };

        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = ZP_ARRAY_SIZE( waitSemaphores );
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.signalSemaphoreCount = ZP_ARRAY_SIZE( signalSemaphores );
        submitInfo.pSignalSemaphores = signalSemaphores;


        HR( vkResetFences( m_vkLocalDevice, 1, &frameData.vkInFlightFences ) );

        VkCommandBuffer graphicsQueueCommandBuffers[commandQueueCount];
        for( zp_size_t i = 0; i < commandQueueCount; ++i )
        {
            graphicsQueueCommandBuffers[ i ] = static_cast<VkCommandBuffer>( frameData.commandQueues[ i ].commandBuffer );
        }
        submitInfo.commandBufferCount = commandQueueCount;
        submitInfo.pCommandBuffers = graphicsQueueCommandBuffers;

        HR( vkQueueSubmit( m_vkRenderQueues[ ZP_RENDER_QUEUE_GRAPHICS ], 1, &submitInfo, frameData.vkInFlightFences ) );
    }

    void VulkanGraphicsDevice::present( zp_uint64_t frameIndex )
    {
        ZP_PROFILE_CPU_BLOCK();

        const zp_uint64_t frame = frameIndex & ( kBufferedFrameCount - 1 );

        VkSemaphore waitSemaphores[] { m_perFrameData[ frame ].vkRenderFinishedSemaphore };

        VkSwapchainKHR swapChains[] { m_vkSwapChain };
        uint32_t imageIndices[] { m_perFrameData[ frame ].swapChainImageIndex };
        ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( swapChains ) == ZP_ARRAY_SIZE( imageIndices ) );

        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = ZP_ARRAY_SIZE( waitSemaphores );
        presentInfo.pWaitSemaphores = waitSemaphores;
        presentInfo.swapchainCount = ZP_ARRAY_SIZE( swapChains );
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = imageIndices;

        VkResult result;
        result = vkQueuePresentKHR( m_vkRenderQueues[ ZP_RENDER_QUEUE_PRESENT ], &presentInfo );
        if( result != VK_SUCCESS )
        {
            if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR )
            {
                // TODO: rebuild swap chain
            }
            else
            {
                ZP_INVALID_CODE_PATH();
            }
        }
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
                0,
                m_vkSwapChainFormat,
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            }
        };

        VkAttachmentReference colorRef {};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;

        VkSubpassDescription subPass {};
        subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subPass.colorAttachmentCount = 1;
        subPass.pColorAttachments = &colorRef;

        VkSubpassDependency subPassDependency {};
        subPassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subPassDependency.dstSubpass = 0;
        subPassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subPassDependency.srcAccessMask = 0;
        subPassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subPassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassCreateInfo {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.pAttachments = attachmentDescriptions;
        renderPassCreateInfo.attachmentCount = ZP_ARRAY_SIZE( attachmentDescriptions );
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subPass;
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &subPassDependency;

        VkRenderPass vkRenderPass;
        HR( vkCreateRenderPass( m_vkLocalDevice, &renderPassCreateInfo, nullptr, &vkRenderPass ) );

        renderPass->internalRenderPass = vkRenderPass;

#if ZP_DEBUG
        if( renderPassDesc->name )
        {
            VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfoExt {};
            debugUtilsObjectNameInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            debugUtilsObjectNameInfoExt.objectType = VK_OBJECT_TYPE_RENDER_PASS;
            debugUtilsObjectNameInfoExt.objectHandle = reinterpret_cast<uint64_t>(vkRenderPass);
            debugUtilsObjectNameInfoExt.pObjectName = renderPassDesc->name;

            HR( CallDebugUtilResult( vkSetDebugUtilsObjectNameEXT, m_vkInstance, m_vkLocalDevice, &debugUtilsObjectNameInfoExt ) );
        }
#endif
    }

    void VulkanGraphicsDevice::destroyRenderPass( RenderPass* renderPass )
    {
        vkDestroyRenderPass( m_vkLocalDevice, static_cast<VkRenderPass>( renderPass->internalRenderPass), nullptr );
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

        VkPipelineShaderStageCreateInfo shaderStageCreateInfos[graphicsPipelineStateCreateDesc->shaderStageCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->shaderStageCount; ++i )
        {
            const ShaderResourceHandle& srcShaderStage = graphicsPipelineStateCreateDesc->shaderStages[ i ];

            VkPipelineShaderStageCreateInfo& shaderStageCreateInfo = shaderStageCreateInfos[ i ];
            shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageCreateInfo.pNext = nullptr;
            shaderStageCreateInfo.flags = 0;
            shaderStageCreateInfo.stage = Convert( srcShaderStage->shaderStage );
            shaderStageCreateInfo.module = static_cast<VkShaderModule>( srcShaderStage->shader );
            shaderStageCreateInfo.pName = srcShaderStage->entryPointName;
            shaderStageCreateInfo.pSpecializationInfo = nullptr;

            vertexShaderUsed |= shaderStageCreateInfo.stage & VK_SHADER_STAGE_VERTEX_BIT;
            tessellationControlShaderUsed |= shaderStageCreateInfo.stage & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            tessellationEvaluationShaderUsed |= shaderStageCreateInfo.stage & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            geometryShaderUsed |= shaderStageCreateInfo.stage & VK_SHADER_STAGE_GEOMETRY_BIT;
            fragmentShaderUsed |= shaderStageCreateInfo.stage & VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        VkVertexInputBindingDescription vertexInputBindingDescriptions[graphicsPipelineStateCreateDesc->vertexBindingCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->vertexBindingCount; ++i )
        {
            const VertexBinding& srcVertexBinding = graphicsPipelineStateCreateDesc->vertexBindings[ i ];

            VkVertexInputBindingDescription& dstVertexBinding = vertexInputBindingDescriptions[ i ];
            dstVertexBinding.binding = srcVertexBinding.binding;
            dstVertexBinding.stride = srcVertexBinding.stride;
            dstVertexBinding.inputRate = Convert( srcVertexBinding.inputRate );
        }

        VkVertexInputAttributeDescription vertexInputAttributeDescriptions[graphicsPipelineStateCreateDesc->vertexAttributeCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->vertexAttributeCount; ++i )
        {
            const VertexAttribute& srcVertexAttribute = graphicsPipelineStateCreateDesc->vertexAttributes[ i ];

            VkVertexInputAttributeDescription& dstVertexAttribute = vertexInputAttributeDescriptions[ i ];
            dstVertexAttribute.location = srcVertexAttribute.location;
            dstVertexAttribute.binding = srcVertexAttribute.binding;
            dstVertexAttribute.format = Convert( srcVertexAttribute.format );
            dstVertexAttribute.offset = srcVertexAttribute.offset;
        }

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = graphicsPipelineStateCreateDesc->vertexBindingCount;
        vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindingDescriptions;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = graphicsPipelineStateCreateDesc->vertexAttributeCount;
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {};
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = graphicsPipelineStateCreateDesc->primitiveRestartEnable;
        inputAssemblyStateCreateInfo.topology = Convert( graphicsPipelineStateCreateDesc->primitiveTopology );

        VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {};
        tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tessellationStateCreateInfo.patchControlPoints = graphicsPipelineStateCreateDesc->patchControlPoints;

        VkViewport viewports[graphicsPipelineStateCreateDesc->viewportCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->viewportCount; ++i )
        {
            const Viewport& srcViewport = graphicsPipelineStateCreateDesc->viewports[ i ];

            VkViewport& dstViewport = viewports[ i ];
            dstViewport.x = srcViewport.x;
            dstViewport.y = srcViewport.y;
            dstViewport.width = srcViewport.width;
            dstViewport.height = srcViewport.height;
            dstViewport.minDepth = srcViewport.minDepth;
            dstViewport.maxDepth = srcViewport.maxDepth;
        }

        VkRect2D scissorRects[graphicsPipelineStateCreateDesc->scissorRectCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->scissorRectCount; ++i )
        {
            const ScissorRect& srcScissor = graphicsPipelineStateCreateDesc->scissorRects[ i ];

            VkRect2D& dstScissor = scissorRects[ i ];
            dstScissor.offset.x = srcScissor.x;
            dstScissor.offset.y = srcScissor.y;
            dstScissor.extent.width = srcScissor.width;
            dstScissor.extent.height = srcScissor.height;
        }

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo {};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = graphicsPipelineStateCreateDesc->viewportCount;
        viewportStateCreateInfo.pViewports = viewports;
        viewportStateCreateInfo.scissorCount = graphicsPipelineStateCreateDesc->scissorRectCount;
        viewportStateCreateInfo.pScissors = scissorRects;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {};
        rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.depthClampEnable = graphicsPipelineStateCreateDesc->depthClampEnabled;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = graphicsPipelineStateCreateDesc->rasterizerDiscardEnable;
        rasterizationStateCreateInfo.polygonMode = Convert( graphicsPipelineStateCreateDesc->polygonFillMode );
        rasterizationStateCreateInfo.cullMode = Convert( graphicsPipelineStateCreateDesc->cullMode );
        rasterizationStateCreateInfo.frontFace = Convert( graphicsPipelineStateCreateDesc->frontFaceMode );
        rasterizationStateCreateInfo.depthBiasEnable = graphicsPipelineStateCreateDesc->depthBiasEnable;
        rasterizationStateCreateInfo.depthBiasConstantFactor = graphicsPipelineStateCreateDesc->depthBiasConstantFactor;
        rasterizationStateCreateInfo.depthBiasClamp = graphicsPipelineStateCreateDesc->depthBiasClamp;
        rasterizationStateCreateInfo.depthBiasSlopeFactor = graphicsPipelineStateCreateDesc->depthBiasSlopeFactor;
        rasterizationStateCreateInfo.lineWidth = graphicsPipelineStateCreateDesc->lineWidth;

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.rasterizationSamples = Convert( graphicsPipelineStateCreateDesc->sampleCount );
        multisampleStateCreateInfo.sampleShadingEnable = graphicsPipelineStateCreateDesc->sampleShadingEnable;
        multisampleStateCreateInfo.minSampleShading = graphicsPipelineStateCreateDesc->minSampleShading;
        multisampleStateCreateInfo.pSampleMask = nullptr;
        multisampleStateCreateInfo.alphaToCoverageEnable = graphicsPipelineStateCreateDesc->alphaToCoverageEnable;
        multisampleStateCreateInfo.alphaToOneEnable = graphicsPipelineStateCreateDesc->alphaToOneEnable;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.depthTestEnable = graphicsPipelineStateCreateDesc->depthTestEnable;
        depthStencilStateCreateInfo.depthWriteEnable = graphicsPipelineStateCreateDesc->depthWriteEnable;
        depthStencilStateCreateInfo.depthCompareOp = Convert( graphicsPipelineStateCreateDesc->depthCompare );
        depthStencilStateCreateInfo.depthBoundsTestEnable = graphicsPipelineStateCreateDesc->depthBoundsTestEnable;
        depthStencilStateCreateInfo.stencilTestEnable = graphicsPipelineStateCreateDesc->stencilTestEnable;

        depthStencilStateCreateInfo.front.failOp = Convert( graphicsPipelineStateCreateDesc->front.fail );
        depthStencilStateCreateInfo.front.passOp = Convert( graphicsPipelineStateCreateDesc->front.pass );
        depthStencilStateCreateInfo.front.depthFailOp = Convert( graphicsPipelineStateCreateDesc->front.depthFail );
        depthStencilStateCreateInfo.front.compareOp = Convert( graphicsPipelineStateCreateDesc->front.compare );
        depthStencilStateCreateInfo.front.compareMask = graphicsPipelineStateCreateDesc->front.compareMask;
        depthStencilStateCreateInfo.front.writeMask = graphicsPipelineStateCreateDesc->front.writeMask;
        depthStencilStateCreateInfo.front.reference = graphicsPipelineStateCreateDesc->front.reference;

        depthStencilStateCreateInfo.back.failOp = Convert( graphicsPipelineStateCreateDesc->back.fail );
        depthStencilStateCreateInfo.back.passOp = Convert( graphicsPipelineStateCreateDesc->back.pass );
        depthStencilStateCreateInfo.back.depthFailOp = Convert( graphicsPipelineStateCreateDesc->back.depthFail );
        depthStencilStateCreateInfo.back.compareOp = Convert( graphicsPipelineStateCreateDesc->back.compare );
        depthStencilStateCreateInfo.back.compareMask = graphicsPipelineStateCreateDesc->back.compareMask;
        depthStencilStateCreateInfo.back.writeMask = graphicsPipelineStateCreateDesc->back.writeMask;
        depthStencilStateCreateInfo.back.reference = graphicsPipelineStateCreateDesc->back.reference;

        depthStencilStateCreateInfo.minDepthBounds = graphicsPipelineStateCreateDesc->minDepthBounds;
        depthStencilStateCreateInfo.maxDepthBounds = graphicsPipelineStateCreateDesc->maxDepthBounds;

        VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[graphicsPipelineStateCreateDesc->blendStateCount];
        for( zp_size_t i = 0; i < graphicsPipelineStateCreateDesc->blendStateCount; ++i )
        {
            const BlendState& srcBlendState = graphicsPipelineStateCreateDesc->blendStates[ i ];

            VkPipelineColorBlendAttachmentState& dstBlendState = colorBlendAttachmentStates[ i ];
            dstBlendState.blendEnable = srcBlendState.blendEnable;
            dstBlendState.srcColorBlendFactor = Convert( srcBlendState.srcColorBlendFactor );
            dstBlendState.dstColorBlendFactor = Convert( srcBlendState.dstColorBlendFactor );
            dstBlendState.colorBlendOp = Convert( srcBlendState.colorBlendOp );
            dstBlendState.srcAlphaBlendFactor = Convert( srcBlendState.srcAlphaBlendFactor );
            dstBlendState.dstAlphaBlendFactor = Convert( srcBlendState.dstAlphaBlendFactor );
            dstBlendState.alphaBlendOp = Convert( srcBlendState.alphaBlendOp );
            dstBlendState.colorWriteMask = Convert( srcBlendState.writeMask );
        }

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable = graphicsPipelineStateCreateDesc->blendLogicalOpEnable;
        colorBlendStateCreateInfo.logicOp = Convert( graphicsPipelineStateCreateDesc->blendLogicalOp );
        colorBlendStateCreateInfo.attachmentCount = graphicsPipelineStateCreateDesc->blendStateCount;
        colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates;
        colorBlendStateCreateInfo.blendConstants[ 0 ] = graphicsPipelineStateCreateDesc->blendConstants[ 0 ];
        colorBlendStateCreateInfo.blendConstants[ 1 ] = graphicsPipelineStateCreateDesc->blendConstants[ 1 ];
        colorBlendStateCreateInfo.blendConstants[ 2 ] = graphicsPipelineStateCreateDesc->blendConstants[ 2 ];
        colorBlendStateCreateInfo.blendConstants[ 3 ] = graphicsPipelineStateCreateDesc->blendConstants[ 3 ];

        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {};
        graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCreateInfo.flags = 0;
        graphicsPipelineCreateInfo.stageCount = graphicsPipelineStateCreateDesc->shaderStageCount;
        graphicsPipelineCreateInfo.pStages = shaderStageCreateInfos;
        graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        graphicsPipelineCreateInfo.pTessellationState = &tessellationStateCreateInfo;
        graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        graphicsPipelineCreateInfo.pDynamicState = nullptr;
        graphicsPipelineCreateInfo.layout = static_cast<VkPipelineLayout>( graphicsPipelineStateCreateDesc->layout->layout );
        graphicsPipelineCreateInfo.renderPass = static_cast<VkRenderPass>( graphicsPipelineStateCreateDesc->renderPass->internalRenderPass );
        graphicsPipelineCreateInfo.subpass = graphicsPipelineStateCreateDesc->subPass;
        graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        graphicsPipelineCreateInfo.basePipelineIndex = -1;

        VkPipeline pipeline;
        HR( vkCreateGraphicsPipelines( m_vkLocalDevice, m_vkPipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline ) );
        graphicsPipelineState->pipelineState = pipeline;
        graphicsPipelineState->pipelineHash = zp_fnv128_1a( &graphicsPipelineCreateInfo, sizeof( VkGraphicsPipelineCreateInfo ) );

#if ZP_DEBUG
        if( graphicsPipelineStateCreateDesc->name )
        {
            VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfoExt {};
            debugUtilsObjectNameInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            debugUtilsObjectNameInfoExt.objectType = VK_OBJECT_TYPE_PIPELINE;
            debugUtilsObjectNameInfoExt.objectHandle = reinterpret_cast<uint64_t>(pipeline);
            debugUtilsObjectNameInfoExt.pObjectName = graphicsPipelineStateCreateDesc->name;

            HR( CallDebugUtilResult( vkSetDebugUtilsObjectNameEXT, m_vkInstance, m_vkLocalDevice, &debugUtilsObjectNameInfoExt ) );
        }
#endif
    }

    void VulkanGraphicsDevice::destroyGraphicsPipeline( GraphicsPipelineState* graphicsPipelineState )
    {
        vkDestroyPipeline( m_vkLocalDevice, static_cast<VkPipeline>( graphicsPipelineState->pipelineState ), nullptr );
        graphicsPipelineState->pipelineState = nullptr;
    }

    void VulkanGraphicsDevice::createPipelineLayout( const PipelineLayoutDesc* pipelineLayoutDesc, PipelineLayout* pipelineLayout )
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        VkPipelineLayout layout;
        HR( vkCreatePipelineLayout( m_vkLocalDevice, &pipelineLayoutCreateInfo, nullptr, &layout ) );
        pipelineLayout->layout = layout;

#if ZP_DEBUG
        if( pipelineLayoutDesc->name )
        {
            VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfoExt {};
            debugUtilsObjectNameInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            debugUtilsObjectNameInfoExt.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
            debugUtilsObjectNameInfoExt.objectHandle = reinterpret_cast<uint64_t>(layout);
            debugUtilsObjectNameInfoExt.pObjectName = pipelineLayoutDesc->name;

            HR( CallDebugUtilResult( vkSetDebugUtilsObjectNameEXT, m_vkInstance, m_vkLocalDevice, &debugUtilsObjectNameInfoExt ) );
        }
#endif
    }

    void VulkanGraphicsDevice::destroyPipelineLayout( PipelineLayout* pipelineLayout )
    {
        vkDestroyPipelineLayout( m_vkLocalDevice, static_cast<VkPipelineLayout>( pipelineLayout->layout ), nullptr );
        pipelineLayout->layout = nullptr;
    }

    void VulkanGraphicsDevice::createBuffer( const GraphicsBufferDesc* graphicsBufferDesc, GraphicsBuffer* graphicsBuffer )
    {
        VkBufferCreateInfo bufferCreateInfo {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = graphicsBufferDesc->size;
        bufferCreateInfo.usage = Convert( graphicsBufferDesc->graphicsBufferUsageFlags );
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer;
        HR( vkCreateBuffer( m_vkLocalDevice, &bufferCreateInfo, nullptr, &buffer ) );

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements( m_vkLocalDevice, buffer, &memoryRequirements );

        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties( m_vkPhysicalDevice, &physicalDeviceMemoryProperties );

        VkMemoryAllocateInfo memoryAllocateInfo {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex( &physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, Convert( graphicsBufferDesc->memoryPropertyFlags ) );

        VkDeviceMemory deviceMemory;
        HR( vkAllocateMemory( m_vkLocalDevice, &memoryAllocateInfo, nullptr, &deviceMemory ) );

        HR( vkBindBufferMemory( m_vkLocalDevice, buffer, deviceMemory, 0 ) );

        graphicsBuffer->buffer = buffer;
        graphicsBuffer->deviceMemory = deviceMemory;
        graphicsBuffer->usageFlags = graphicsBufferDesc->graphicsBufferUsageFlags;
        graphicsBuffer->offset = 0;
        graphicsBuffer->size = memoryRequirements.size;
        graphicsBuffer->alignment = memoryRequirements.alignment;
        graphicsBuffer->isVirtualBuffer = false;

#if ZP_DEBUG
        if( graphicsBufferDesc->name )
        {
            VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfoExt {};
            debugUtilsObjectNameInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            debugUtilsObjectNameInfoExt.objectType = VK_OBJECT_TYPE_BUFFER;
            debugUtilsObjectNameInfoExt.objectHandle = reinterpret_cast<uint64_t>(buffer);
            debugUtilsObjectNameInfoExt.pObjectName = graphicsBufferDesc->name;

            HR( CallDebugUtilResult( vkSetDebugUtilsObjectNameEXT, m_vkInstance, m_vkLocalDevice, &debugUtilsObjectNameInfoExt ) );
        }
#endif
    }

    void VulkanGraphicsDevice::destroyBuffer( GraphicsBuffer* graphicsBuffer )
    {
        if( !graphicsBuffer->isVirtualBuffer )
        {
            if( graphicsBuffer->buffer )
            {
                vkDestroyBuffer( m_vkLocalDevice, static_cast<VkBuffer>( graphicsBuffer->buffer ), nullptr );
            }

            if( graphicsBuffer->deviceMemory )
            {
                vkFreeMemory( m_vkLocalDevice, static_cast<VkDeviceMemory>( graphicsBuffer->deviceMemory ), nullptr );

            }
        }

        graphicsBuffer->buffer = nullptr;
        graphicsBuffer->deviceMemory = nullptr;

        graphicsBuffer->offset = 0;
        graphicsBuffer->size = 0;
        graphicsBuffer->alignment = 0;

        graphicsBuffer->usageFlags = {};
    }

    void VulkanGraphicsDevice::createShader( const ShaderDesc* shaderDesc, Shader* shader )
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo {};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = shaderDesc->codeSize << 2; // codeSize in bytes -> uint
        shaderModuleCreateInfo.pCode = static_cast<const uint32_t*>( shaderDesc->codeData );

        VkShaderModule shaderModule;
        HR( vkCreateShaderModule( m_vkLocalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule ) );
        shader->shader = shaderModule;
        shader->shaderStage = shaderDesc->shaderStage;
        shader->shaderHash = zp_fnv128_1a( shaderDesc->codeData, shaderDesc->codeSize );

        const zp_size_t length = ZP_ARRAY_SIZE( shader->entryPointName );
        zp_memcpy( shader->entryPointName, length, shaderDesc->entryPointName, zp_strnlen( shaderDesc->entryPointName, length ) );

#if ZP_DEBUG
        if( shaderDesc->name )
        {
            VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfoExt {};
            debugUtilsObjectNameInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            debugUtilsObjectNameInfoExt.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
            debugUtilsObjectNameInfoExt.objectHandle = reinterpret_cast<uint64_t>(shaderModule);
            debugUtilsObjectNameInfoExt.pObjectName = shaderDesc->name;

            HR( CallDebugUtilResult( vkSetDebugUtilsObjectNameEXT, m_vkInstance, m_vkLocalDevice, &debugUtilsObjectNameInfoExt ) );
        }
#endif
    }

    void VulkanGraphicsDevice::destroyShader( Shader* shader )
    {
        vkDestroyShaderModule( m_vkLocalDevice, static_cast<VkShaderModule>( shader->shader ), nullptr );
        shader->shader = nullptr;
    }

    void VulkanGraphicsDevice::createTexture( const TextureCreateDesc* textureCreateDesc, Texture* texture )
    {
        VkImageCreateInfo imageCreateInfo {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = Convert( textureCreateDesc->textureDimension );
        imageCreateInfo.format = Convert( textureCreateDesc->textureFormat );
        imageCreateInfo.extent.width = textureCreateDesc->size.width;
        imageCreateInfo.extent.height = textureCreateDesc->size.height;
        imageCreateInfo.extent.depth = IsTextureArray( textureCreateDesc->textureDimension ) ? textureCreateDesc->size.depth : 1;
        imageCreateInfo.mipLevels = textureCreateDesc->mipCount;
        imageCreateInfo.arrayLayers = textureCreateDesc->arrayLayers;
        imageCreateInfo.samples = Convert( textureCreateDesc->samples );
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = Convert( textureCreateDesc->usage );
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.queueFamilyIndexCount = 0;
        imageCreateInfo.pQueueFamilyIndices = nullptr;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImage image;
        HR( vkCreateImage( m_vkLocalDevice, &imageCreateInfo, nullptr, &image ) );

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements( m_vkLocalDevice, image, &memoryRequirements );

        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties( m_vkPhysicalDevice, &physicalDeviceMemoryProperties );

        VkMemoryAllocateInfo memoryAllocateInfo {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex( &physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, Convert( textureCreateDesc->memoryPropertyFlags ) );

        VkDeviceMemory deviceMemory;
        HR( vkAllocateMemory( m_vkLocalDevice, &memoryAllocateInfo, nullptr, &deviceMemory ) );

        HR( vkBindImageMemory( m_vkLocalDevice, image, deviceMemory, 0 ) );

        VkImageViewCreateInfo imageViewCreateInfo {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.viewType = ConvertImageView( textureCreateDesc->textureDimension );
        imageViewCreateInfo.format = imageCreateInfo.format;
        imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        imageViewCreateInfo.subresourceRange = {
            ConvertAspect( textureCreateDesc->usage ),
            0,
            textureCreateDesc->mipCount,
            0,
            textureCreateDesc->arrayLayers
        };

        VkImageView imageView;
        HR( vkCreateImageView( m_vkLocalDevice, &imageViewCreateInfo, nullptr, &imageView ) );

        texture->textureDimension = textureCreateDesc->textureDimension;
        texture->textureFormat = textureCreateDesc->textureFormat;
        texture->usage = textureCreateDesc->usage;

        texture->size = textureCreateDesc->size;

        texture->textureHandle = image;
        texture->textureViewHandle = imageView;
        texture->textureMemoryHandle = deviceMemory;
    }

    void VulkanGraphicsDevice::destroyTexture( Texture* texture )
    {
        if( texture->textureViewHandle )
        {
            vkDestroyImageView( m_vkLocalDevice, static_cast<VkImageView>(texture->textureViewHandle), nullptr );
        }

        if( texture->textureMemoryHandle )
        {
            vkFreeMemory( m_vkLocalDevice, static_cast<VkDeviceMemory>( texture->textureMemoryHandle ), nullptr );
        }

        if( texture->textureHandle )
        {
            vkDestroyImage( m_vkLocalDevice, static_cast<VkImage>( texture->textureHandle ), nullptr );
        }

        texture->textureHandle = nullptr;
        texture->textureMemoryHandle = nullptr;
        texture->textureViewHandle = nullptr;
    }

    void VulkanGraphicsDevice::createSampler( const SamplerCreateDesc* samplerCreateDesc, Sampler* sampler )
    {
        VkSamplerCreateInfo samplerCreateInfo {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = Convert( samplerCreateDesc->magFilter );
        samplerCreateInfo.minFilter = Convert( samplerCreateDesc->minFilter );
        samplerCreateInfo.mipmapMode = Convert( samplerCreateDesc->mipmapMode );
        samplerCreateInfo.addressModeU = Convert( samplerCreateDesc->addressModeU );
        samplerCreateInfo.addressModeV = Convert( samplerCreateDesc->addressModeV );
        samplerCreateInfo.addressModeW = Convert( samplerCreateDesc->addressModeW );
        samplerCreateInfo.mipLodBias = samplerCreateDesc->mipLodBias;
        samplerCreateInfo.anisotropyEnable = samplerCreateDesc->anisotropyEnabled;
        samplerCreateInfo.maxAnisotropy = samplerCreateDesc->maxAnisotropy;
        samplerCreateInfo.compareEnable = samplerCreateDesc->compareEnabled;
        samplerCreateInfo.compareOp = Convert( samplerCreateDesc->compareOp );
        samplerCreateInfo.minLod = samplerCreateDesc->minLod;
        samplerCreateInfo.maxLod = samplerCreateDesc->maxLod;
        samplerCreateInfo.borderColor = Convert( samplerCreateDesc->borderColor );
        samplerCreateInfo.unnormalizedCoordinates = samplerCreateDesc->unnormalizedCoordinates;

        VkSampler vkSampler;
        HR( vkCreateSampler( m_vkLocalDevice, &samplerCreateInfo, nullptr, &vkSampler ) );

        sampler->samplerHandle = vkSampler;
    }

    void VulkanGraphicsDevice::destroySampler( Sampler* sampler )
    {
        if( sampler->samplerHandle )
        {
            vkDestroySampler( m_vkLocalDevice, static_cast<VkSampler>( sampler->samplerHandle ), nullptr );
        }

        sampler->samplerHandle = nullptr;
    }

    void VulkanGraphicsDevice::mapBuffer( zp_size_t offset, zp_size_t size, GraphicsBuffer* graphicsBuffer, void** memory )
    {
        VkDeviceMemory deviceMemory = static_cast<VkDeviceMemory>( graphicsBuffer->deviceMemory );
        vkMapMemory( m_vkLocalDevice, deviceMemory, offset + graphicsBuffer->offset, size, 0, memory );
    }

    void VulkanGraphicsDevice::unmapBuffer( GraphicsBuffer* graphicsBuffer )
    {
        VkDeviceMemory deviceMemory = static_cast<VkDeviceMemory>( graphicsBuffer->deviceMemory );
        vkUnmapMemory( m_vkLocalDevice, deviceMemory );
    }

    CommandQueue* VulkanGraphicsDevice::requestCommandQueue( RenderQueue queue, zp_uint64_t frameIndex )
    {
        queue = ZP_RENDER_QUEUE_GRAPHICS;

        const zp_size_t frame = frameIndex & ( kBufferedFrameCount - 1 );
        PerFrameData& frameData = m_perFrameData[ frame ];

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

                ZP_FREE_( memoryLabel, frameData.commandQueues );
                frameData.commandQueues = nullptr;
            }

            frameData.commandQueues = newCommandQueues;
            frameData.commandQueueCapacity = newCommandQueueCapacity;
        }

        if( commandQueueIndex > 0 )
        {
            const zp_size_t prevCommandQueueIndex = commandQueueIndex - 1;
            CommandQueue& prevCommandQueue = frameData.commandQueues[ prevCommandQueueIndex ];
            VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>( prevCommandQueue.commandBuffer );

#if ZP_USE_PROFILER
            if( prevCommandQueue.queue != ZP_RENDER_QUEUE_TRANSFER )
            {
                vkCmdWriteTimestamp( commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frameData.vkTimestampQueryPool, prevCommandQueueIndex * 2 + 1 );
                //vkCmdEndQuery( commandBuffer, frameData.vkPipelineStatisticsQueryPool, 0 );
            }
#endif
            HR( vkEndCommandBuffer( commandBuffer ) );
        }

        const VkCommandPool commandPoolMap[] {
            m_vkGraphicsCommandPool,
            m_vkTransferCommandPool,
            m_vkComputeCommandPool,
            m_vkGraphicsCommandPool
        };

        CommandQueue* commandQueue = &frameData.commandQueues[ commandQueueIndex ];
        if( commandQueue->commandBuffer != VK_NULL_HANDLE && commandQueue->queue != queue )
        {
            auto commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
            vkFreeCommandBuffers( m_vkLocalDevice, commandPoolMap[ commandQueue->queue ], 1, &commandBuffer );
        }

        commandQueue->queue = queue;
        commandQueue->frame = frame;
        commandQueue->frameIndex = frameIndex;

        if( commandQueue->commandBuffer == nullptr )
        {
            VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
            commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocateInfo.commandPool = commandPoolMap[ queue ];
            commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            HR( vkAllocateCommandBuffers( m_vkLocalDevice, &commandBufferAllocateInfo, &commandBuffer ) );
            commandQueue->commandBuffer = commandBuffer;
        }

        VkCommandBufferBeginInfo commandBufferBeginInfo {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkCommandBuffer buffer = static_cast<VkCommandBuffer>( commandQueue->commandBuffer );
        HR( vkBeginCommandBuffer( buffer, &commandBufferBeginInfo ) );

#if ZP_USE_PROFILER
        if( queue != ZP_RENDER_QUEUE_TRANSFER )
        {
            vkCmdResetQueryPool( buffer, frameData.vkTimestampQueryPool, commandQueueIndex * 2, 2 );
            vkCmdWriteTimestamp( buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameData.vkTimestampQueryPool, commandQueueIndex * 2 );
        }

        //vkCmdBeginQuery( buffer, frameData.vkPipelineStatisticsQueryPool, 0, 0 );
#endif

        return commandQueue;
    }

    void VulkanGraphicsDevice::beginRenderPass( const RenderPass* renderPass, CommandQueue* commandQueue )
    {
        VkClearValue clearValues[] {
            { .color = { 0, 0, 0, 1 } }
        };

        VkRenderPassBeginInfo renderPassBeginInfo {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass ? static_cast<VkRenderPass>( renderPass->internalRenderPass ) : m_vkSwapChainRenderPass;
        renderPassBeginInfo.framebuffer = m_swapChainFrameBuffers[ m_perFrameData[ commandQueue->frame ].swapChainImageIndex ];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = m_vkSwapChainExtent;
        renderPassBeginInfo.clearValueCount = ZP_ARRAY_SIZE( clearValues );
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass( static_cast<VkCommandBuffer>(commandQueue->commandBuffer), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
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
        const VkDeviceSize bufferOffset = offset + graphicsBuffer->offset;
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
        GraphicsBufferAllocation allocation = m_perFrameData[ commandQueue->frame ].perFrameStagingBuffer.allocate( textureUpdateDesc->textureDataSize );

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
            VkBufferMemoryBarrier memoryBarrier {};
            memoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            memoryBarrier.buffer = static_cast<VkBuffer>( allocation.buffer );
            memoryBarrier.offset = allocation.offset;
            memoryBarrier.size = allocation.size;

            VkImageMemoryBarrier imageMemoryBarrier {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.image = static_cast<VkImage>( dstTexture->textureHandle );
            imageMemoryBarrier.subresourceRange = {
                ConvertAspect( dstTexture->usage ),
                textureUpdateDesc->minMipLevel,
                mipCount,
                0,
                1
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
                    ConvertAspect( dstTexture->usage ),
                    mip,
                    0,
                    1
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
            VkImageMemoryBarrier imageMemoryBarrier {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.image = static_cast<VkImage>( dstTexture->textureHandle );
            imageMemoryBarrier.subresourceRange = {
                ConvertAspect( dstTexture->usage ),
                textureUpdateDesc->minMipLevel,
                mipCount,
                0,
                1
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
        GraphicsBufferAllocation allocation = m_perFrameData[ commandQueue->frame ].perFrameStagingBuffer.allocate( graphicsBufferUpdateDesc->dataSize );

        auto commandBuffer = static_cast<VkCommandBuffer>( commandQueue->commandBuffer );

        // copy data to staging buffer
        {
            auto deviceMemory = static_cast<VkDeviceMemory>( allocation.deviceMemory );

            void* dstMemory {};
            HR( vkMapMemory( m_vkLocalDevice, deviceMemory, allocation.offset, allocation.size, 0, &dstMemory ) );

            zp_memcpy( dstMemory, allocation.size, graphicsBufferUpdateDesc->data, graphicsBufferUpdateDesc->dataSize );

            VkMappedMemoryRange flushMemoryRange {};
            flushMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            flushMemoryRange.memory = static_cast<VkDeviceMemory>(allocation.deviceMemory);
            flushMemoryRange.offset = allocation.offset;
            flushMemoryRange.size = allocation.size;
            vkFlushMappedMemoryRanges( m_vkLocalDevice, 1, &flushMemoryRange );

            vkUnmapMemory( m_vkLocalDevice, deviceMemory );
        }

        // transfer from CPU to GPU memory
        if( false )
        {
            VkBufferMemoryBarrier memoryBarrier[2] {};
            memoryBarrier[ 0 ].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            memoryBarrier[ 0 ].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            memoryBarrier[ 0 ].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            memoryBarrier[ 0 ].buffer = static_cast<VkBuffer>( allocation.buffer );
            memoryBarrier[ 0 ].offset = allocation.offset;
            memoryBarrier[ 0 ].size = allocation.size;

            memoryBarrier[ 1 ].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            memoryBarrier[ 1 ].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            memoryBarrier[ 1 ].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            memoryBarrier[ 1 ].buffer = static_cast<VkBuffer>( dstGraphicsBuffer->buffer );
            memoryBarrier[ 1 ].offset = dstGraphicsBuffer->offset;
            memoryBarrier[ 1 ].size = allocation.size;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                0,
                0,
                nullptr,
                2,
                memoryBarrier,
                0,
                nullptr
            );
        }

        // copy staging buffer to dst buffer
        {
            VkBufferCopy bufferCopy {};
            bufferCopy.srcOffset = allocation.offset;
            bufferCopy.dstOffset = dstGraphicsBuffer->offset;
            bufferCopy.size = allocation.size;

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
            VkBufferMemoryBarrier memoryBarrier {};
            memoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            memoryBarrier.buffer = static_cast<VkBuffer>( dstGraphicsBuffer->buffer );
            memoryBarrier.offset = dstGraphicsBuffer->offset;
            memoryBarrier.size = allocation.size;

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

    void VulkanGraphicsDevice::drawIndexed( zp_uint32_t indexCount, zp_uint32_t instanceCount, zp_uint32_t firstIndex, zp_int32_t vertexOffset, zp_uint32_t firstInstance, CommandQueue* commandQueue )
    {
        VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(commandQueue->commandBuffer);
        vkCmdDrawIndexed( commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
    }

    void VulkanGraphicsDevice::beginEventLabel( const char* eventLabel, const Color& color, CommandQueue* commandQueue )
    {
#if ZP_DEBUG
        VkDebugMarkerMarkerInfoEXT debugMarkerMarkerInfo {};
        debugMarkerMarkerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
        debugMarkerMarkerInfo.pMarkerName = eventLabel;
        debugMarkerMarkerInfo.color[ 0 ] = color.r;
        debugMarkerMarkerInfo.color[ 1 ] = color.g;
        debugMarkerMarkerInfo.color[ 2 ] = color.b;
        debugMarkerMarkerInfo.color[ 3 ] = color.a;

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
        VkDebugMarkerMarkerInfoEXT debugMarkerMarkerInfo {};
        debugMarkerMarkerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
        debugMarkerMarkerInfo.pMarkerName = eventLabel;
        debugMarkerMarkerInfo.color[ 0 ] = color.r;
        debugMarkerMarkerInfo.color[ 1 ] = color.g;
        debugMarkerMarkerInfo.color[ 2 ] = color.b;
        debugMarkerMarkerInfo.color[ 3 ] = color.a;

        CallDebugUtil( vkCmdDebugMarkerInsertEXT, m_vkInstance, static_cast<VkCommandBuffer>(commandQueue->commandBuffer), &debugMarkerMarkerInfo );
#endif
    }

}