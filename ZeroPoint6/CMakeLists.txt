
set(ZP_VERSION_MAJOR 6)
set(ZP_VERSION_MINOR 0)
set(ZP_VERSION_PATCH 0)
set(ZP_VERSION ${ZP_VERSION_MAJOR}.${ZP_VERSION_MINOR}.${ZP_VERSION_PATCH})

project(ZeroPoint VERSION ${ZP_VERSION} LANGUAGES CXX)

set(ZP_TARGET ${PROJECT_NAME}_${PLATFORM_NAME}_${PLATFORM_ARCH})

set(ZP_RENDERING_API Vulkan)
find_package(Vulkan REQUIRED)

# Source Files
set(ZP_CORE_SRC
        "src/Core/Allocator.cpp"
        "src/Core/Common.cpp"
        "src/Core/Math.cpp"
        "src/Core/Profiler.cpp"
        )

set(ZP_PLATFORM_SRC
        "src/Platform/${PLATFORM_FULL_NAME}/${PLATFORM_FULL_NAME}Main.cpp"
        "src/Platform/${PLATFORM_FULL_NAME}/${PLATFORM_FULL_NAME}Platform.cpp"
        )

set(ZP_RENDERING_SRC
        "src/Rendering/BatchModeRenderer.cpp"
        "src/Rendering/GraphicsDevice.cpp"
        "src/Rendering/GraphicsResource.cpp"
        "src/Rendering/ImmediateModeRenderer.cpp"
        "src/Rendering/RenderSystem.cpp"
        "src/Rendering/${ZP_RENDERING_API}/${ZP_RENDERING_API}GraphicsDevice.cpp"
        "src/Rendering/${ZP_RENDERING_API}/${ZP_RENDERING_API}RenderingEngine.cpp"
        "src/Rendering/${ZP_RENDERING_API}/${ZP_RENDERING_API}Shader.cpp"
        )

set(ZP_ENGINE_SRC
        "src/Engine/AssetSystem.cpp"
        "src/Engine/Component.cpp"
        "src/Engine/Engine.cpp"
        "src/Engine/Entity.cpp"
        "src/Engine/EntityComponentManager.cpp"
        "src/Engine/EntityQuery.cpp"
        "src/Engine/JobSystem.cpp"
        "src/Engine/Subsystem.cpp"
        )

set(ZP_SRC
        ${ZP_CORE_SRC}
        ${ZP_PLATFORM_SRC}
        ${ZP_ENGINE_SRC}
        ${ZP_RENDERING_SRC})

# Header Files
configure_file(
        "include/Core/Version.h.in"
        "include/Core/Version.h"
        NEWLINE_STYLE UNIX
        @ONLY
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
set(ZP_CORE_HEADERS
        "include/Core/Allocator.h"
        "include/Core/Atomic.h"
        "include/Core/Common.h"
        "include/Core/Defines.h"
        "include/Core/Macros.h"
        "include/Core/Map.h"
        "include/Core/Math.h"
        "include/Core/Profiler.h"
        "include/Core/Queue.h"
        "include/Core/Set.h"
        "include/Core/Types.h"
        "include/Core/Vector.h"
        )

set(ZP_ENGINE_HEADERS
        "include/Engine/ApplicationEntryPoint.h"
        "include/Engine/AssetSystem.h"
        "include/Engine/BaseComponentSystem.h"
        "include/Engine/Component.h"
        "include/Engine/ComponentSignature.h"
        "include/Engine/Engine.h"
        "include/Engine/EnginePipeline.h"
        "include/Engine/Entity.h"
        "include/Engine/EntityComponentManager.h"
        "include/Engine/EntityQuery.h"
        "include/Engine/JobSystem.h"
        "include/Engine/MemoryLabels.h"
        "include/Engine/ModuleEntryPointAPI.h"
        "include/Engine/Subsystem.h"
        "include/Engine/TransformComponent.h"
        )

set(ZP_PLATFORM_HEADERS
        "include/Platform/Platform.h"
        )

set(ZP_RENDERING_HEADERS
        "include/Rendering/BatchModeRenderer.h"
        "include/Rendering/Camera.h"
        "include/Rendering/GraphicsDevice.h"
        "include/Rendering/GraphicsResource.h"
        "include/Rendering/ImmediateModeRenderer.h"
        "include/Rendering/Material.h"
        "include/Rendering/RenderPass.h"
        "include/Rendering/RenderPipeline.h"
        "include/Rendering/RenderSystem.h"
        "include/Rendering/RenderTarget.h"
        "include/Rendering/Sampler.h"
        "include/Rendering/Shader.h"
        "include/Rendering/Texture.h"
        "include/Rendering/Swapchain.h"
        "include/Rendering/${ZP_RENDERING_API}/${ZP_RENDERING_API}GraphicsDevice.h"
        )

set(ZP_HEADERS
        ${ZP_CORE_HEADERS}
        ${ZP_ENGINE_HEADERS}
        ${ZP_RENDERING_HEADERS}
        )

set(ZP_CORE_MODULE
        "modules/Core/Core.ixx"
        "modules/Core/Core-Types.ixx"
        "modules/Core/Core-Rect.ixx"
        "modules/Core/Core-Vector2.ixx"
        "modules/Core/Core-Printf.ixx"
        )

set(ZP_PLATFORM_MODULE
        "modules/Platform/Platform.ixx"
        "src/Platform/Platform-${PLATFORM_FULL_NAME}.cpp"
        )

set(ZP_EXTERNAL_HEADERS
        "external/tlsf/tlsf.h"
        )

set(ZP_EXTERNAL_SRC
        "external/tlsf/tlsf.c"
        )

add_executable(${ZP_TARGET})
target_sources(${ZP_TARGET}
        PRIVATE ${ZP_HEADERS}
        PRIVATE ${ZP_SRC}
        PRIVATE ${ZP_EXTERNAL_HEADERS}
        PRIVATE ${ZP_EXTERNAL_SRC}
        )
target_include_directories(${ZP_TARGET}
        PRIVATE include
        PRIVATE external
        )

# add_module(CoreModule ${ZP_CORE_MODULE})
# add_module(PlatformModule ${ZP_PLATFORM_MODULE})

target_link_libraries(${ZP_TARGET}
        #PRIVATE CoreModule
        #PRIVATE PlatformModule
        )

#if (${ZP_RENDERING_API} EQUAL Vulkan)
#cmake_path(SET VulkanSDK $ENV{VULKAN_SDK})

target_include_directories(${ZP_TARGET}
        PRIVATE ${Vulkan_INCLUDE_DIRS}
        )
target_link_options(${ZP_TARGET}
        PRIVATE -static-libgcc -static-libstdc++ -static
        )
target_link_libraries(${ZP_TARGET}
        PRIVATE ${Vulkan_LIBRARIES}
#        PRIVATE "${VulkanSDK}/Lib/vulkan-1.lib"
#        PRIVATE "${VulkanSDK}/Lib/VkLayer_utils.lib"
        PRIVATE "rpcrt4.lib"
        )
#endif ()

set_target_properties(${ZP_TARGET} PROPERTIES
        VERSION ${ZP_VERSION})

if (MSVC)
    set_target_properties(${ZP_TARGET} PROPERTIES
            LINK_FLAGS "/SUBSYSTEM:WINDOWS"
            VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
elseif (GCC OR CLANG)
else ()
endif ()

install(TARGETS ${ZP_TARGET}
        RUNTIME DESTINATION ${ROOT_DIR}/bin)
