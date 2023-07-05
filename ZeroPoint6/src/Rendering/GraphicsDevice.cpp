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
    GraphicsDevice* CreateGraphicsDevice( MemoryLabel memoryLabel, const GraphicsDeviceDesc& graphicsDeviceDesc )
    {
#if ZP_RENDERING_VULKAN
        return ZP_NEW_ARGS_( memoryLabel, VulkanGraphicsDevice, graphicsDeviceDesc );
#elif
        return nullptr;
#endif
    }

    void DestroyGraphicsDevice( GraphicsDevice* graphicsDevice )
    {
#if ZP_RENDERING_VULKAN
        auto ptr = reinterpret_cast<VulkanGraphicsDevice*>(graphicsDevice );
        ZP_SAFE_DELETE( VulkanGraphicsDevice, ptr );
#else
#endif
    }
}
