//
// Created by phosg on 11/6/2021.
//

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include <Core/Version.h>
#include "Rendering/RenderSystem.h"

#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR

//#include <vulkan/vulkan.h>
#include <Volk/volk.h>

#if ZP_DEBUG
#define HR( r )   do { if( (r) != VK_SUCCESS ) { printf("HR Failed: " #r ); } } while( false )
#else
#define HR( r )   r
#endif

struct QueueFamilies
{
    uint32_t familyMask;
    uint32_t graphicsFamily;
    uint32_t presentFamily;
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanRenderingEngine
{
public:
    void Resize( uint32_t width, uint32_t height );

private:
};

static VkInstance vkInstance = {};

static VkAllocationCallbacks vkAllocationCallbacks = {};

static VkPhysicalDevice vkPhysicalDevice = {};

static QueueFamilies vkQueueFamilies = {};

static VkDevice vkLocalDevice = {};

static VkQueue vkGraphicsQueue = {};

static VkQueue vkPresentQueue = {};

static VkSurfaceKHR vkSurface = {};

static VkSwapchainKHR vkSwapchain = {};

static std::vector<VkImage> vkSwapchainImages;

static std::vector<VkImageView> vkSwapchainImageViews;

static VkFormat vkSwapchainFormat;

static VkExtent2D vkSwapchainExtent;

static VkRenderPass vkRenderPass;

static VkPipelineLayout vkPipelineLayout;

static VkPipeline vkGraphicsPipeline;

static std::vector<VkFramebuffer> vkSwapchainFrameBuffers;

static VkCommandPool vkCommandPool;

static std::vector<VkCommandBuffer> vkCommandBuffers;

static std::vector<VkSemaphore> vkImageAvailableSemaphores;

static std::vector<VkSemaphore> vkRenderFinishedSemaphores;

static std::vector<VkFence> vkInFlightFences;

static std::vector<VkFence> vkImagesInFlightFences;

static int currentFrame = 0;

static bool frameBufferResized = false;

const int MAX_FRAMES_IN_FLIGHT = 2;

#if ZP_DEBUG

static VkDebugUtilsMessengerEXT vpDebugMessenger = {};

static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger )
{
    auto f = vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(f);
    if( func )
    {
        return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator )
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        instance,
        "vkDestroyDebugUtilsMessengerEXT" ));
    if( func )
    {
        func( instance, debugMessenger, pAllocator );
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData )
{
    const VkDebugUtilsMessageSeverityFlagBitsEXT messageMask = VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT;
    if( ( messageSeverity & messageMask ) == messageSeverity )
    {
        printf( "validation layer: %s\n", pCallbackData->pMessage );
    }

    const VkBool32 shouldAbort = false; // messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? VK_TRUE : VK_FALSE;
    return shouldAbort;
}

#endif // ZP_DEBUG

static SwapChainSupportDetails QuerySwapchainDetails( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface )
{
    SwapChainSupportDetails swapChainSupportDetails;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physicalDevice, surface, &swapChainSupportDetails.capabilities );

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, surface, &formatCount, nullptr );

    if( formatCount != 0 )
    {
        swapChainSupportDetails.formats.resize( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &formatCount,
            swapChainSupportDetails.formats.data() );
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, surface, &presentModeCount, nullptr );

    if( presentModeCount != 0 )
    {
        swapChainSupportDetails.presentModes.resize( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &presentModeCount,
            swapChainSupportDetails.presentModes.data() );
    }

    return swapChainSupportDetails;
}

static bool IsDeviceSuitable( VkPhysicalDevice physicalDevice )
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    vkGetPhysicalDeviceProperties( physicalDevice, &physicalDeviceProperties );
    vkGetPhysicalDeviceFeatures( physicalDevice, &physicalDeviceFeatures );

    const bool isDiscreteGPU = physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    const bool isGeometryShaderSupported = physicalDeviceFeatures.geometryShader == VK_TRUE;

    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &extensionCount, nullptr );

    std::vector<VkExtensionProperties> availableExtensions( extensionCount );
    vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &extensionCount, availableExtensions.data() );

    std::set<std::string> requiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    for( const auto& extension : availableExtensions )
    {
        requiredExtensions.erase( extension.extensionName );
    }

    const bool isExtensionsAvailable = requiredExtensions.empty();

    bool isSwapChainAdequate = false;
    if( isExtensionsAvailable )
    {
        SwapChainSupportDetails swapChainSupportDetails = QuerySwapchainDetails( physicalDevice, vkSurface );
        isSwapChainAdequate = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
    }

    return isDiscreteGPU && isGeometryShaderSupported && isExtensionsAvailable && isSwapChainAdequate;
}

static VkSurfaceFormatKHR ChooseSwapChainSurfaceFormat( const SwapChainSupportDetails& swapChainSupportDetails )
{
    VkSurfaceFormatKHR chosenFormat = swapChainSupportDetails.formats[ 0 ];

    for( const VkSurfaceFormatKHR& format : swapChainSupportDetails.formats )
    {
        if( format.format == VK_FORMAT_B8G8R8A8_SNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            chosenFormat = format;
            break;
        }
    }

    return chosenFormat;
}

static VkPresentModeKHR ChooseSwapChainPresentMode( const SwapChainSupportDetails& swapChainSupportDetails )
{
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    for( const VkPresentModeKHR& present : swapChainSupportDetails.presentModes )
    {
        if( present == VK_PRESENT_MODE_MAILBOX_KHR )
        {
            chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    return chosenPresentMode;
}

static VkExtent2D ChooseSwapChainExtent(
    const SwapChainSupportDetails& swapChainSupportDetails,
    uint32_t requestedWith,
    uint32_t requestedHeight )
{
    VkExtent2D chosenExtents;

    if( swapChainSupportDetails.capabilities.currentExtent.width != UINT32_MAX )
    {
        chosenExtents = swapChainSupportDetails.capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtents = { requestedWith, requestedHeight };

        actualExtents.width = std::clamp(
            actualExtents.width,
            swapChainSupportDetails.capabilities.minImageExtent.width,
            swapChainSupportDetails.capabilities.maxImageExtent.width );
        actualExtents.height = std::clamp(
            actualExtents.height,
            swapChainSupportDetails.capabilities.minImageExtent.height,
            swapChainSupportDetails.capabilities.maxImageExtent.height );

        chosenExtents = actualExtents;
    }

    return chosenExtents;
}

struct InitializeConfig
{
    uint32_t width;
    uint32_t height;
    HINSTANCE hInstance;
    HWND hWnd;
};

#if ZP_DEBUG

static const char* kValidationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};

#endif

static void FillLayerNames( const char* const*& ppEnabledLayerNames, uint32_t& enabledLayerCount )
{
#if ZP_DEBUG
    enabledLayerCount = ZP_ARRAY_SIZE( kValidationLayers );
    ppEnabledLayerNames = kValidationLayers;
#else
    enabledLayerCount = 0;
    ppEnabledLayerNames = nullptr;
#endif
}

static void CreateVulkanInstance()
{
    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "AppName";
    applicationInfo.apiVersion = VK_MAKE_VERSION( 1, 0, 0 );
    applicationInfo.pEngineName = "ZeroPoint6";
    applicationInfo.engineVersion = VK_MAKE_VERSION( ZP_VERSION_MAJOR, ZP_VERSION_MINOR, ZP_VERSION_PATCH );
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#if ZP_DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };

    // create instance
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount = ZP_ARRAY_SIZE( extensions );
    instanceCreateInfo.ppEnabledExtensionNames = extensions;

    FillLayerNames( instanceCreateInfo.ppEnabledLayerNames, instanceCreateInfo.enabledLayerCount );

#if ZP_DEBUG
    // add debug info to create instance
    VkDebugUtilsMessengerCreateInfoEXT createInstanceDebugMessengerInfo = {};
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
    HR( vkCreateInstance( &instanceCreateInfo, nullptr, &vkInstance ) );

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

    HR( CreateDebugUtilsMessengerEXT( vkInstance, &createDebugMessengerInfo, nullptr, &vpDebugMessenger ) );
#endif
}

static void CreateVulkanSurface( HWND hWnd )
{
    auto hInstance = reinterpret_cast<HINSTANCE>(::GetWindowLongPtr( hWnd, GWLP_HINSTANCE ));

    // create surface
    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {};
    win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCreateInfo.hwnd = hWnd;
    win32SurfaceCreateInfo.hinstance = hInstance;

    HR( vkCreateWin32SurfaceKHR( vkInstance, &win32SurfaceCreateInfo, nullptr, &vkSurface ) );
}

static void SelectVulkanPhysicalDevice()
{
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices( vkInstance, &physicalDeviceCount, nullptr );

    std::vector<VkPhysicalDevice> physicalDevices( physicalDeviceCount );
    vkEnumeratePhysicalDevices( vkInstance, &physicalDeviceCount, physicalDevices.data() );

    bool foundPhysicalDevice = false;
    for( auto device : physicalDevices )
    {
        if( IsDeviceSuitable( device ) )
        {
            vkPhysicalDevice = device;
            foundPhysicalDevice = true;
            break;
        }
    }

    if( !foundPhysicalDevice )
    {
        // assert
    }
}

static void CreateVulkanQueueFamilies()
{
    vkQueueFamilies = {};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &queueFamilyCount, nullptr );

    std::vector<VkQueueFamilyProperties> queueFamilyProperties( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data() );

    for( int i = 0; i < queueFamilyProperties.size(); ++i )
    {
        const auto& queueFamily = queueFamilyProperties[ i ];
        if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            vkQueueFamilies.familyMask |= 1 << 0;
            vkQueueFamilies.graphicsFamily = i;

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR( vkPhysicalDevice, i, vkSurface, &presentSupport );

            if( presentSupport )
            {
                vkQueueFamilies.familyMask |= 1 << 1;
                vkQueueFamilies.presentFamily = i;
            }
        }
    }
}

static void CreateVulkanLocalDevice()
{
    float queuePriorities[] = { 1.0f };

    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
    std::set<uint32_t> uniqueFamilyIndices = { vkQueueFamilies.graphicsFamily, vkQueueFamilies.presentFamily };

    for( uint32_t queueFamily : uniqueFamilyIndices )
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = queuePriorities;

        deviceQueueCreateInfos.push_back( queueCreateInfo );
    }

    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkPhysicalDeviceFeatures deviceFeatures = {};
    vkGetPhysicalDeviceFeatures( vkPhysicalDevice, &deviceFeatures );

    VkDeviceCreateInfo localDeviceCreateInfo = {};
    localDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    localDeviceCreateInfo.queueCreateInfoCount = deviceQueueCreateInfos.size();
    localDeviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
    localDeviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    localDeviceCreateInfo.enabledExtensionCount = ZP_ARRAY_SIZE( deviceExtensions );
    localDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    FillLayerNames( localDeviceCreateInfo.ppEnabledLayerNames, localDeviceCreateInfo.enabledLayerCount );

    HR( vkCreateDevice( vkPhysicalDevice, &localDeviceCreateInfo, nullptr, &vkLocalDevice ) );
}

static void GetVulkanDeviceQueues()
{
    vkGetDeviceQueue( vkLocalDevice, vkQueueFamilies.graphicsFamily, 0, &vkGraphicsQueue );
    vkGetDeviceQueue( vkLocalDevice, vkQueueFamilies.presentFamily, 0, &vkPresentQueue );
}

static void CreateVulkanSwapchain()
{
    // create swap chain
    SwapChainSupportDetails swapchainSupportDetails = QuerySwapchainDetails( vkPhysicalDevice, vkSurface );

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapChainSurfaceFormat( swapchainSupportDetails );
    VkPresentModeKHR presentMode = ChooseSwapChainPresentMode( swapchainSupportDetails );
    VkExtent2D extent = ChooseSwapChainExtent(
        swapchainSupportDetails,
        swapchainSupportDetails.capabilities.currentExtent.width,
        swapchainSupportDetails.capabilities.currentExtent.height );

    uint32_t imageCount = swapchainSupportDetails.capabilities.minImageCount + 1;
    if( swapchainSupportDetails.capabilities.maxImageCount > 0 )
    {
        imageCount = imageCount < swapchainSupportDetails.capabilities.maxImageCount ? imageCount : swapchainSupportDetails.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = vkSurface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t queueFamilyIndices[] = { vkQueueFamilies.graphicsFamily, vkQueueFamilies.presentFamily };

    if( vkQueueFamilies.graphicsFamily != vkQueueFamilies.presentFamily )
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = ZP_ARRAY_SIZE( queueFamilyIndices );
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
        swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    swapchainCreateInfo.preTransform = swapchainSupportDetails.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    HR( vkCreateSwapchainKHR( vkLocalDevice, &swapchainCreateInfo, nullptr, &vkSwapchain ) );

    vkSwapchainFormat = surfaceFormat.format;
    vkSwapchainExtent = extent;
}

static void CreateVulkanSwapchainImages()
{
    uint32_t swapchainImageCount = 0;
    vkGetSwapchainImagesKHR( vkLocalDevice, vkSwapchain, &swapchainImageCount, nullptr );

    vkSwapchainImages.resize( swapchainImageCount, VK_NULL_HANDLE );
    vkGetSwapchainImagesKHR( vkLocalDevice, vkSwapchain, &swapchainImageCount, vkSwapchainImages.data() );

    // create swap chain image views
    vkSwapchainImageViews.resize( vkSwapchainImages.size(), VK_NULL_HANDLE );
    for( int i = 0; i < vkSwapchainImages.size(); ++i )
    {
        VkImageViewCreateInfo imageViewCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vkSwapchainImages[ i ],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vkSwapchainFormat,
            .components {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1, }
        };

        HR( vkCreateImageView( vkLocalDevice, &imageViewCreateInfo, nullptr, &vkSwapchainImageViews[ i ] ) );
    }
}

static void CreateVulkanRenderPasses()
{
    // create render passes
    VkAttachmentDescription colorAttachmentDescription {
        .format = vkSwapchainFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpassDescription {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
    };

    VkSubpassDependency subpassDependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachmentDescription,
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency,
    };

    HR( vkCreateRenderPass( vkLocalDevice, &renderPassCreateInfo, nullptr, &vkRenderPass ) );
}

static void CreateVulkanGraphicsPipeline()
{
    // create graphics pipeline(s)
    const uint32_t vertexShaderCode[] = { 0x07230203, 0x00010000, 0x000d000a, 0x00000036,
                                          0x00000000, 0x00020011, 0x00000001, 0x0006000b,
                                          0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
                                          0x00000000, 0x0003000e, 0x00000000, 0x00000001,
                                          0x0008000f, 0x00000000, 0x00000004, 0x6e69616d,
                                          0x00000000, 0x00000022, 0x00000026, 0x00000031,
                                          0x00030003, 0x00000002, 0x000001c2, 0x000a0004,
                                          0x475f4c47, 0x4c474f4f, 0x70635f45, 0x74735f70,
                                          0x5f656c79, 0x656e696c, 0x7269645f, 0x69746365,
                                          0x00006576, 0x00080004, 0x475f4c47, 0x4c474f4f,
                                          0x6e695f45, 0x64756c63, 0x69645f65, 0x74636572,
                                          0x00657669, 0x00040005, 0x00000004, 0x6e69616d,
                                          0x00000000, 0x00050005, 0x0000000c, 0x69736f70,
                                          0x6e6f6974, 0x00000073, 0x00040005, 0x00000017,
                                          0x6f6c6f63, 0x00007372, 0x00060005, 0x00000020,
                                          0x505f6c67, 0x65567265, 0x78657472, 0x00000000,
                                          0x00060006, 0x00000020, 0x00000000, 0x505f6c67,
                                          0x7469736f, 0x006e6f69, 0x00070006, 0x00000020,
                                          0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953,
                                          0x00000000, 0x00070006, 0x00000020, 0x00000002,
                                          0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e,
                                          0x00070006, 0x00000020, 0x00000003, 0x435f6c67,
                                          0x446c6c75, 0x61747369, 0x0065636e, 0x00030005,
                                          0x00000022, 0x00000000, 0x00060005, 0x00000026,
                                          0x565f6c67, 0x65747265, 0x646e4978, 0x00007865,
                                          0x00050005, 0x00000031, 0x67617266, 0x6f6c6f43,
                                          0x00000072, 0x00050048, 0x00000020, 0x00000000,
                                          0x0000000b, 0x00000000, 0x00050048, 0x00000020,
                                          0x00000001, 0x0000000b, 0x00000001, 0x00050048,
                                          0x00000020, 0x00000002, 0x0000000b, 0x00000003,
                                          0x00050048, 0x00000020, 0x00000003, 0x0000000b,
                                          0x00000004, 0x00030047, 0x00000020, 0x00000002,
                                          0x00040047, 0x00000026, 0x0000000b, 0x0000002a,
                                          0x00040047, 0x00000031, 0x0000001e, 0x00000000,
                                          0x00020013, 0x00000002, 0x00030021, 0x00000003,
                                          0x00000002, 0x00030016, 0x00000006, 0x00000020,
                                          0x00040017, 0x00000007, 0x00000006, 0x00000002,
                                          0x00040015, 0x00000008, 0x00000020, 0x00000000,
                                          0x0004002b, 0x00000008, 0x00000009, 0x00000003,
                                          0x0004001c, 0x0000000a, 0x00000007, 0x00000009,
                                          0x00040020, 0x0000000b, 0x00000006, 0x0000000a,
                                          0x0004003b, 0x0000000b, 0x0000000c, 0x00000006,
                                          0x0004002b, 0x00000006, 0x0000000d, 0x00000000,
                                          0x0004002b, 0x00000006, 0x0000000e, 0xbf000000,
                                          0x0005002c, 0x00000007, 0x0000000f, 0x0000000d,
                                          0x0000000e, 0x0004002b, 0x00000006, 0x00000010,
                                          0x3f000000, 0x0005002c, 0x00000007, 0x00000011,
                                          0x00000010, 0x00000010, 0x0005002c, 0x00000007,
                                          0x00000012, 0x0000000e, 0x00000010, 0x0006002c,
                                          0x0000000a, 0x00000013, 0x0000000f, 0x00000011,
                                          0x00000012, 0x00040017, 0x00000014, 0x00000006,
                                          0x00000003, 0x0004001c, 0x00000015, 0x00000014,
                                          0x00000009, 0x00040020, 0x00000016, 0x00000006,
                                          0x00000015, 0x0004003b, 0x00000016, 0x00000017,
                                          0x00000006, 0x0004002b, 0x00000006, 0x00000018,
                                          0x3f800000, 0x0006002c, 0x00000014, 0x00000019,
                                          0x00000018, 0x0000000d, 0x0000000d, 0x0006002c,
                                          0x00000014, 0x0000001a, 0x0000000d, 0x00000018,
                                          0x0000000d, 0x0006002c, 0x00000014, 0x0000001b,
                                          0x0000000d, 0x0000000d, 0x00000018, 0x0006002c,
                                          0x00000015, 0x0000001c, 0x00000019, 0x0000001a,
                                          0x0000001b, 0x00040017, 0x0000001d, 0x00000006,
                                          0x00000004, 0x0004002b, 0x00000008, 0x0000001e,
                                          0x00000001, 0x0004001c, 0x0000001f, 0x00000006,
                                          0x0000001e, 0x0006001e, 0x00000020, 0x0000001d,
                                          0x00000006, 0x0000001f, 0x0000001f, 0x00040020,
                                          0x00000021, 0x00000003, 0x00000020, 0x0004003b,
                                          0x00000021, 0x00000022, 0x00000003, 0x00040015,
                                          0x00000023, 0x00000020, 0x00000001, 0x0004002b,
                                          0x00000023, 0x00000024, 0x00000000, 0x00040020,
                                          0x00000025, 0x00000001, 0x00000023, 0x0004003b,
                                          0x00000025, 0x00000026, 0x00000001, 0x00040020,
                                          0x00000028, 0x00000006, 0x00000007, 0x00040020,
                                          0x0000002e, 0x00000003, 0x0000001d, 0x00040020,
                                          0x00000030, 0x00000003, 0x00000014, 0x0004003b,
                                          0x00000030, 0x00000031, 0x00000003, 0x00040020,
                                          0x00000033, 0x00000006, 0x00000014, 0x00050036,
                                          0x00000002, 0x00000004, 0x00000000, 0x00000003,
                                          0x000200f8, 0x00000005, 0x0003003e, 0x0000000c,
                                          0x00000013, 0x0003003e, 0x00000017, 0x0000001c,
                                          0x0004003d, 0x00000023, 0x00000027, 0x00000026,
                                          0x00050041, 0x00000028, 0x00000029, 0x0000000c,
                                          0x00000027, 0x0004003d, 0x00000007, 0x0000002a,
                                          0x00000029, 0x00050051, 0x00000006, 0x0000002b,
                                          0x0000002a, 0x00000000, 0x00050051, 0x00000006,
                                          0x0000002c, 0x0000002a, 0x00000001, 0x00070050,
                                          0x0000001d, 0x0000002d, 0x0000002b, 0x0000002c,
                                          0x0000000d, 0x00000018, 0x00050041, 0x0000002e,
                                          0x0000002f, 0x00000022, 0x00000024, 0x0003003e,
                                          0x0000002f, 0x0000002d, 0x0004003d, 0x00000023,
                                          0x00000032, 0x00000026, 0x00050041, 0x00000033,
                                          0x00000034, 0x00000017, 0x00000032, 0x0004003d,
                                          0x00000014, 0x00000035, 0x00000034, 0x0003003e,
                                          0x00000031, 0x00000035, 0x000100fd, 0x00010038 };

    VkShaderModuleCreateInfo vertexShaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = ZP_ARRAY_SIZE( vertexShaderCode ) * 4,
        .pCode = vertexShaderCode,
    };

    VkShaderModule vertexShaderModule;
    HR( vkCreateShaderModule( vkLocalDevice, &vertexShaderModuleCreateInfo, nullptr, &vertexShaderModule ) );

    const uint32_t fragmentShaderCode[] = { 0x07230203, 0x00010000, 0x000d000a, 0x00000013,
                                            0x00000000, 0x00020011, 0x00000001, 0x0006000b,
                                            0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
                                            0x00000000, 0x0003000e, 0x00000000, 0x00000001,
                                            0x0007000f, 0x00000004, 0x00000004, 0x6e69616d,
                                            0x00000000, 0x00000009, 0x0000000c, 0x00030010,
                                            0x00000004, 0x00000007, 0x00030003, 0x00000002,
                                            0x000001c2, 0x000a0004, 0x475f4c47, 0x4c474f4f,
                                            0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c,
                                            0x7269645f, 0x69746365, 0x00006576, 0x00080004,
                                            0x475f4c47, 0x4c474f4f, 0x6e695f45, 0x64756c63,
                                            0x69645f65, 0x74636572, 0x00657669, 0x00040005,
                                            0x00000004, 0x6e69616d, 0x00000000, 0x00050005,
                                            0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000,
                                            0x00050005, 0x0000000c, 0x67617266, 0x6f6c6f43,
                                            0x00000072, 0x00040047, 0x00000009, 0x0000001e,
                                            0x00000000, 0x00040047, 0x0000000c, 0x0000001e,
                                            0x00000000, 0x00020013, 0x00000002, 0x00030021,
                                            0x00000003, 0x00000002, 0x00030016, 0x00000006,
                                            0x00000020, 0x00040017, 0x00000007, 0x00000006,
                                            0x00000004, 0x00040020, 0x00000008, 0x00000003,
                                            0x00000007, 0x0004003b, 0x00000008, 0x00000009,
                                            0x00000003, 0x00040017, 0x0000000a, 0x00000006,
                                            0x00000003, 0x00040020, 0x0000000b, 0x00000001,
                                            0x0000000a, 0x0004003b, 0x0000000b, 0x0000000c,
                                            0x00000001, 0x0004002b, 0x00000006, 0x0000000e,
                                            0x3f800000, 0x00050036, 0x00000002, 0x00000004,
                                            0x00000000, 0x00000003, 0x000200f8, 0x00000005,
                                            0x0004003d, 0x0000000a, 0x0000000d, 0x0000000c,
                                            0x00050051, 0x00000006, 0x0000000f, 0x0000000d,
                                            0x00000000, 0x00050051, 0x00000006, 0x00000010,
                                            0x0000000d, 0x00000001, 0x00050051, 0x00000006,
                                            0x00000011, 0x0000000d, 0x00000002, 0x00070050,
                                            0x00000007, 0x00000012, 0x0000000f, 0x00000010,
                                            0x00000011, 0x0000000e, 0x0003003e, 0x00000009,
                                            0x00000012, 0x000100fd, 0x00010038 };


    VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = ZP_ARRAY_SIZE( fragmentShaderCode ) * 4,
        .pCode = fragmentShaderCode,
    };

    VkShaderModule fragmentShaderModule;
    HR( vkCreateShaderModule( vkLocalDevice, &fragmentShaderModuleCreateInfo, nullptr, &fragmentShaderModule ) );

    // shaderHandle stages
    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertexShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragmentShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

    // fixed function pipelines
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr, // Optional
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr, // Optional
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) vkSwapchainExtent.width,
        .height = (float) vkSwapchainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = vkSwapchainExtent,
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Optional
        .depthBiasClamp = 0.0f, // Optional
        .depthBiasSlopeFactor = 0.0f, // Optional
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f, // Optional
        .pSampleMask = nullptr, // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable = VK_FALSE, // Optional
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .colorBlendOp = VK_BLEND_OP_ADD, // Optional
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .alphaBlendOp = VK_BLEND_OP_ADD, // Optional
        .colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
    colorBlendStateCreateInfo.blendConstants[ 0 ] = 0.0f; // Optional
    colorBlendStateCreateInfo.blendConstants[ 1 ] = 0.0f; // Optional
    colorBlendStateCreateInfo.blendConstants[ 2 ] = 0.0f; // Optional
    colorBlendStateCreateInfo.blendConstants[ 3 ] = 0.0f; // Optional

    VkDynamicState dynamicStates[] {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = ZP_ARRAY_SIZE( dynamicStates ),
        .pDynamicStates = dynamicStates,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0, // Optional
        .pSetLayouts = nullptr, // Optional
        .pushConstantRangeCount = 0, // Optional
        .pPushConstantRanges = nullptr, // Optional
    };

    HR( vkCreatePipelineLayout( vkLocalDevice, &pipelineLayoutCreateInfo, nullptr, &vkPipelineLayout ) );

    VkGraphicsPipelineCreateInfo pipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = ZP_ARRAY_SIZE( shaderStages ),
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = nullptr, // Optional
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = nullptr, //&dynamicStateCreateInfo,
        .layout = vkPipelineLayout,
        .renderPass = vkRenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    HR( vkCreateGraphicsPipelines(
        vkLocalDevice,
        VK_NULL_HANDLE,
        1,
        &pipelineCreateInfo,
        nullptr,
        &vkGraphicsPipeline ) );
}

static void CreateVulkanFramebuffers()
{
    // create frame buffers
    vkSwapchainFrameBuffers.resize( vkSwapchainImages.size(), VK_NULL_HANDLE );

    for( int i = 0; i < vkSwapchainImageViews.size(); ++i )
    {
        VkImageView attachments[] = { vkSwapchainImageViews[ i ] };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = vkRenderPass;
        framebufferCreateInfo.attachmentCount = ZP_ARRAY_SIZE( attachments );
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = vkSwapchainExtent.width;
        framebufferCreateInfo.height = vkSwapchainExtent.height;
        framebufferCreateInfo.layers = 1;

        HR( vkCreateFramebuffer( vkLocalDevice, &framebufferCreateInfo, nullptr, &vkSwapchainFrameBuffers[ i ] ) );
    }
}

static void CreateVulkanCommandPool()
{
    // create command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = vkQueueFamilies.graphicsFamily;
    commandPoolCreateInfo.flags = 0;

    HR( vkCreateCommandPool( vkLocalDevice, &commandPoolCreateInfo, nullptr, &vkCommandPool ) );
}

static void CreateVulkanCommandBuffers()
{
    // create command buffers
    vkCommandBuffers.resize( vkSwapchainFrameBuffers.size(), VK_NULL_HANDLE );

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = vkCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = vkCommandBuffers.size();

    HR( vkAllocateCommandBuffers( vkLocalDevice, &commandBufferAllocateInfo, vkCommandBuffers.data() ) );
}

static void CreateVulkanSemaphores()
{
    // create semaphores
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkImageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE );
    vkRenderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE );
    vkInFlightFences.resize( MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE );
    vkImagesInFlightFences.resize( vkSwapchainImages.size(), VK_NULL_HANDLE );

    for( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        HR( vkCreateSemaphore( vkLocalDevice, &semaphoreCreateInfo, nullptr, &vkImageAvailableSemaphores[ i ] ) );
        HR( vkCreateSemaphore( vkLocalDevice, &semaphoreCreateInfo, nullptr, &vkRenderFinishedSemaphores[ i ] ) );
        HR( vkCreateFence( vkLocalDevice, &fenceCreateInfo, nullptr, &vkInFlightFences[ i ] ) );
    }
}

static void FillCommandBuffersWithTempData()
{
    for( int i = 0; i < vkCommandBuffers.size(); ++i )
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = 0; // Optional
        commandBufferBeginInfo.pInheritanceInfo = nullptr; // Optional

        HR( vkBeginCommandBuffer( vkCommandBuffers[ i ], &commandBufferBeginInfo ) );

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = vkRenderPass;
        renderPassBeginInfo.framebuffer = vkSwapchainFrameBuffers[ i ];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = vkSwapchainExtent;

        VkClearValue clearColor[] = { { 0, 0, 0, 1 } };
        renderPassBeginInfo.clearValueCount = ZP_ARRAY_SIZE( clearColor );
        renderPassBeginInfo.pClearValues = clearColor;

        vkCmdBeginRenderPass( vkCommandBuffers[ i ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

        vkCmdBindPipeline( vkCommandBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline );

        vkCmdDraw( vkCommandBuffers[ i ], 3, 1, 0, 0 );

        vkCmdEndRenderPass( vkCommandBuffers[ i ] );

        HR( vkEndCommandBuffer( vkCommandBuffers[ i ] ) );
    }
}

static void DestorySwapchain()
{
    for( auto swapchainFramebuffer : vkSwapchainFrameBuffers )
    {
        vkDestroyFramebuffer( vkLocalDevice, swapchainFramebuffer, nullptr );
    }

    vkFreeCommandBuffers( vkLocalDevice, vkCommandPool, vkCommandBuffers.size(), vkCommandBuffers.data() );

    vkDestroyPipeline( vkLocalDevice, vkGraphicsPipeline, nullptr );
    vkGraphicsPipeline = {};

    vkDestroyPipelineLayout( vkLocalDevice, vkPipelineLayout, nullptr );
    vkPipelineLayout = {};

    vkDestroyRenderPass( vkLocalDevice, vkRenderPass, nullptr );
    vkRenderPass = {};

    for( auto swapchainImageView : vkSwapchainImageViews )
    {
        vkDestroyImageView( vkLocalDevice, swapchainImageView, nullptr );
    }

    vkDestroySwapchainKHR( vkLocalDevice, vkSwapchain, nullptr );
    vkSwapchain = {};
}

static void RecreateSwapchain()
{
    vkDeviceWaitIdle( vkLocalDevice );

    DestorySwapchain();

    CreateVulkanSwapchain();

    CreateVulkanSwapchainImages();

    CreateVulkanRenderPasses();

    CreateVulkanGraphicsPipeline();

    CreateVulkanFramebuffers();

    CreateVulkanCommandBuffers();

    FillCommandBuffersWithTempData();
}

static void Initialize( const InitializeConfig* configData )
{
    frameBufferResized = false;

    CreateVulkanInstance();

    CreateVulkanSurface( configData->hWnd );

    SelectVulkanPhysicalDevice();

    CreateVulkanQueueFamilies();

    CreateVulkanLocalDevice();

    GetVulkanDeviceQueues();

    CreateVulkanSwapchain();

    CreateVulkanSwapchainImages();

    CreateVulkanRenderPasses();

    CreateVulkanGraphicsPipeline();

    CreateVulkanFramebuffers();

    CreateVulkanCommandPool();

    CreateVulkanCommandBuffers();

    CreateVulkanSemaphores();

    FillCommandBuffersWithTempData();

    /*
    // get valid extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());

    // get valid validationLayers
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());


    VkResult result;

    // select physical device

    uint32_t physicalDeviceGroupCount = 0;
    vkEnumeratePhysicalDeviceGroups(vkInstance, &physicalDeviceGroupCount, nullptr);

    std::vector<VkPhysicalDeviceGroupProperties> physicalDeviceGroups(physicalDeviceGroupCount);
    vkEnumeratePhysicalDeviceGroups(vkInstance, &physicalDeviceGroupCount, physicalDeviceGroups.data());
    */

}

static void Destroy()
{
    vkDeviceWaitIdle( vkLocalDevice );

    DestorySwapchain();

    for( VkSemaphore semaphore : vkRenderFinishedSemaphores )
    {
        vkDestroySemaphore( vkLocalDevice, semaphore, nullptr );
    }
    vkRenderFinishedSemaphores.clear();

    for( VkSemaphore semaphore : vkImageAvailableSemaphores )
    {
        vkDestroySemaphore( vkLocalDevice, semaphore, nullptr );
    }
    vkImageAvailableSemaphores.clear();

    for( VkFence fence : vkInFlightFences )
    {
        vkDestroyFence( vkLocalDevice, fence, nullptr );
    }
    vkInFlightFences.clear();

    vkDestroyCommandPool( vkLocalDevice, vkCommandPool, nullptr );
    vkCommandPool = {};
    vkCommandBuffers.clear();

    vkDestroyDevice( vkLocalDevice, nullptr );
    vkLocalDevice = {};
    vkPhysicalDevice = {};

    vkDestroySurfaceKHR( vkInstance, vkSurface, nullptr );
    vkSurface = {};

#if ZP_DEBUG
    DestroyDebugUtilsMessengerEXT( vkInstance, vpDebugMessenger, nullptr );
    vpDebugMessenger = {};
#endif

    vkDestroyInstance( vkInstance, nullptr );
    vkInstance = {};
    vkAllocationCallbacks = {};
}

static void DrawFrame()
{
    VkResult result;

    vkWaitForFences( vkLocalDevice, 1, &vkInFlightFences[ currentFrame ], VK_TRUE, UINT64_MAX );

    uint32_t imageIndex;
    result = vkAcquireNextImageKHR(
        vkLocalDevice,
        vkSwapchain,
        UINT64_MAX,
        vkImageAvailableSemaphores[ currentFrame ],
        VK_NULL_HANDLE,
        &imageIndex );
    if( result == VK_ERROR_OUT_OF_DATE_KHR )
    {
        // rebuild swap
        RecreateSwapchain();
        return;
    }
    else if( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR )
    {
        // assert
        DebugBreak();
        ZP_ASSERT( false );
    }

    if( vkImagesInFlightFences[ imageIndex ] != VK_NULL_HANDLE )
    {
        vkWaitForFences( vkLocalDevice, 1, &vkImagesInFlightFences[ imageIndex ], VK_TRUE, UINT64_MAX );
    }

    vkImagesInFlightFences[ imageIndex ] = vkInFlightFences[ currentFrame ];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { vkImageAvailableSemaphores[ currentFrame ] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = ZP_ARRAY_SIZE( waitSemaphores );
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCommandBuffers[ imageIndex ];

    VkSemaphore signalSemaphores[] = { vkRenderFinishedSemaphores[ currentFrame ] };
    submitInfo.signalSemaphoreCount = ZP_ARRAY_SIZE( signalSemaphores );
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences( vkLocalDevice, 1, &vkInFlightFences[ currentFrame ] );

    HR( vkQueueSubmit( vkGraphicsQueue, 1, &submitInfo, vkInFlightFences[ currentFrame ] ) );

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = ZP_ARRAY_SIZE( signalSemaphores );
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { vkSwapchain };
    presentInfo.swapchainCount = ZP_ARRAY_SIZE( swapChains );
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR( vkPresentQueue, &presentInfo );
    if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized )
    {
        frameBufferResized = false;
        // rebuild swap
        RecreateSwapchain();
    }
    else if( result != VK_SUCCESS )
    {
        // assert
        ZP_ASSERT( false );
    }

    currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
}

namespace zp
{
    void InitializeRenderingEngine( void* hinst, void* hwnd, int width, int height )
    {
        InitializeConfig config = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            static_cast<HINSTANCE>(hinst),
            static_cast<HWND>(hwnd),
        };

        Initialize( &config );
    }

    void DestroyRenderingEngine()
    {
        Destroy();
    }

    void RenderFrame()
    {
        DrawFrame();
    }

    void ResizeRenderingEngine( uint32_t width, uint32_t height )
    {
        frameBufferResized = true;
    }
}