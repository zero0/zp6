//
// Created by phosg on 1/31/2022.
//
#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

#include "Engine/MemoryLabels.h"

#include "Rendering/GraphicsDevice.h"

#define ZP_RENDERING_VULKAN     ZP_RENDERING_API == Vulkan

#if ZP_RENDERING_VULKAN

#include "Rendering/Vulkan/VulkanGraphicsDevice.h"

#else
#error "Unsupported Graphics API"
#endif

namespace zp
{
    GraphicsDevice* CreateGraphicsDevice( MemoryLabel memoryLabel, GraphicsDeviceFeatures graphicsDeviceFeatures )
    {
#if ZP_RENDERING_VULKAN
        return ZP_NEW_ARGS_( memoryLabel, VulkanGraphicsDevice, graphicsDeviceFeatures );
#elif
        return nullptr;
#endif
    }

    void DestroyGraphicsDevice( GraphicsDevice* graphicsDevice )
    {
        ZP_SAFE_DELETE( GraphicsDevice, graphicsDevice );
    }

    GraphicsDevice::GraphicsDevice( MemoryLabel memoryLabel )
        : memoryLabel( memoryLabel )
    {
    }

    GraphicsDevice::~GraphicsDevice()
    {
    }
}
