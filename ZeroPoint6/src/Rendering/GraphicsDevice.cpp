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

#define ZP_RENDERING_API_VULKAN     ZP_RENDERING_API == Vulkan
#define ZP_RENDERING_API_D3D12      ZP_RENDERING_API == D3D12

#if ZP_RENDERING_API_VULKAN
#include "Rendering/Vulkan/VulkanGraphicsDevice.h"
#elif ZP_RENDERING_API_D3D12
#error "Unsupported Graphics API D3D12"
#else
#error "Unknown Graphics API"
#endif

namespace zp
{
    GraphicsDevice* CreateGraphicsDevice( MemoryLabel memoryLabel )
    {
#if ZP_RENDERING_API_VULKAN
        return internal::CreateVulkanGraphicsDevice( memoryLabel );
#elif ZP_RENDERING_API_D3D12
        ZP_INVALID_CODE_PATH_MSG("Graphics API D3D12 not supported");
        return nullptr;
#elif
        ZP_INVALID_CODE_PATH_MSG("No Graphics API Defined");
        return nullptr;
#endif
    }

    void DestroyGraphicsDevice( GraphicsDevice* graphicsDevice )
    {
#if ZP_RENDERING_API_VULKAN
        internal::DestroyVulkanGraphicsDevice( graphicsDevice );
#elif ZP_RENDERING_API_D3D12
        ZP_INVALID_CODE_PATH_MSG("Graphics API D3D12 not supported");
#else
        ZP_INVALID_CODE_PATH_MSG("No Graphics API Defined");
#endif
    }
}
