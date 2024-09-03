
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
        "src/Core/CommandLine.cpp"
        "src/Core/Common.cpp"
        "src/Core/Data.cpp"
        "src/Core/Job.cpp"
        "src/Core/Log.cpp"
        "src/Core/Math.cpp"
        "src/Core/Profiler.cpp"
        "src/Core/String.cpp"
        "src/Core/Threading.cpp"
        )

set(ZP_PLATFORM_SRC
        "src/Platform/${PLATFORM_FULL_NAME}/Platform${PLATFORM_FULL_NAME}.cpp"
        )

set(ZP_RENDERING_SRC
        "src/Rendering/BatchModeRenderer.cpp"
        "src/Rendering/GraphicsDevice.cpp"
        "src/Rendering/GraphicsResource.cpp"
        "src/Rendering/ImmediateModeRenderer.cpp"
        "src/Rendering/RenderGraph.cpp"
        "src/Rendering/RenderSystem.cpp"
        )

set(ZP_RENDERING_API_SRC
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
        "src/Engine/ExecutionGraph.cpp"
        "src/Engine/Subsystem.cpp"
        "src/Engine/${PLATFORM_FULL_NAME}/Main${PLATFORM_FULL_NAME}.cpp"
        )

set(ZP_SRC
        ${ZP_CORE_SRC}
        ${ZP_PLATFORM_SRC}
        ${ZP_ENGINE_SRC}
        ${ZP_RENDERING_SRC}
        ${ZP_RENDERING_API_SRC}
        )

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
        "include/Core/CommandLine.h"
        "include/Core/Common.h"
        "include/Core/Data.h"
        "include/Core/Defines.h"
        "include/Core/Function.h"
        "include/Core/Job.h"
        "include/Core/Log.h"
        "include/Core/Macros.h"
        "include/Core/Map.h"
        "include/Core/Math.h"
        "include/Core/Profiler.h"
        "include/Core/Queue.h"
        "include/Core/Set.h"
        "include/Core/String.h"
        "include/Core/Threading.h"
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
        "include/Engine/ExecutionGraph.h"
        "include/Engine/MemoryLabels.h"
        "include/Engine/ModuleEntryPointAPI.h"
        "include/Engine/Subsystem.h"
        "include/Engine/TransformComponent.h"
        )

set(ZP_ENTRY_POINT_HEADERS
        "include/EntryPoint/EntryPoint.h"
        )

set(ZP_PLATFORM_HEADERS
        "include/Platform/Platform.h"
        )

set(ZP_RENDERING_HEADERS
        "include/Rendering/BatchModeRenderer.h"
        "include/Rendering/Camera.h"
        "include/Rendering/GraphicsBuffer.h"
        "include/Rendering/GraphicsDevice.h"
        "include/Rendering/GraphicsResource.h"
        "include/Rendering/ImmediateModeRenderer.h"
        "include/Rendering/Material.h"
        "include/Rendering/RenderGraph.h"
        "include/Rendering/RenderPass.h"
        "include/Rendering/RenderPipeline.h"
        "include/Rendering/RenderSystem.h"
        "include/Rendering/RenderTarget.h"
        "include/Rendering/Sampler.h"
        "include/Rendering/Shader.h"
        "include/Rendering/Texture.h"
        "include/Rendering/Swapchain.h"
        )

set(ZP_RENDERING_API_HEADERS
        "include/Rendering/${ZP_RENDERING_API}/${ZP_RENDERING_API}GraphicsDevice.h"
        )

set(ZP_HEADERS
        ${ZP_CORE_HEADERS}
        ${ZP_ENGINE_HEADERS}
        ${ZP_ENTRY_POINT}
        ${ZP_PLATFORM_HEADERS}
        ${ZP_RENDERING_HEADERS}
        ${ZP_RENDERING_API_HEADERS}
        )

# set(ZP_CORE_MODULE
#         "modules/Core/Core.ixx"
#         "modules/Core/Core-Types.ixx"
#         "modules/Core/Core-Rect.ixx"
#         "modules/Core/Core-Vector2.ixx"
#         "modules/Core/Core-Printf.ixx"
#         )

# set(ZP_PLATFORM_MODULE
#         "modules/Platform/Platform.ixx"
#         "src/Platform/Platform-${PLATFORM_FULL_NAME}.cpp"
#         )

set(ZP_EXTERNAL_HEADERS
        "external/tlsf/tlsf.h"
        )

set(ZP_EXTERNAL_SRC
        "external/tlsf/tlsf.c"
        )

#if (GCC OR CLANG)
#target_compile_definitions(${ZP_TARGET}}
#        PUBLIC -D_WIN32_WINNT=0x0A00
#        )
#endif ()

add_executable(${ZP_TARGET}
                "res/${PLATFORM_FULL_NAME}/resource.rc")
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

# target_link_libraries(${ZP_TARGET}
#         PRIVATE CoreModule
#         PRIVATE PlatformModule
#         )

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
        PRIVATE "rpcrt4.lib"
        PRIVATE "dwmapi.dll"
        PRIVATE "Ws2_32.lib"
        )
#endif ()

set_target_properties(${ZP_TARGET} PROPERTIES
        VERSION ${ZP_VERSION}
        )

if (MSVC)
    set_target_properties(${ZP_TARGET} PROPERTIES
            LINK_FLAGS "/SUBSYSTEM:WINDOWS"
            VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            )
    set_target_properties(${ZP_TOOLS_TARGET} PROPERTIES
            LINK_FLAGS "/SUBSYSTEM:WINDOWS"
            VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            )
elseif (GCC OR CLANG)
else ()
endif ()

install(TARGETS ${ZP_TARGET}
        CONFIGURATIONS Debug
        RUNTIME DESTINATION ${ROOT_DIR}/Debug/bin
        )
install(TARGETS ${ZP_TARGET}
        CONFIGURATIONS Release
        RUNTIME DESTINATION ${ROOT_DIR}/Release/bin
        )

#
#
#

project(ZeroPointTools VERSION ${ZP_VERSION} LANGUAGES CXX)

find_package(Vulkan REQUIRED COMPONENTS dxc)

set(ZP_TOOLS_TARGET ${PROJECT_NAME}_${PLATFORM_NAME}_${PLATFORM_ARCH})

set(ZP_TOOLS_SRC
        "src/Tools/AssetCompiler.cpp"
        "src/Tools/${PLATFORM_FULL_NAME}/AssetCompiler${PLATFORM_FULL_NAME}Main.cpp"
        "src/Tools/${PLATFORM_FULL_NAME}/ShaderCompiler${PLATFORM_FULL_NAME}.cpp"
)

set(ZP_TOOLS_HEADERS
        "include/Tools/AssetCompiler.h"
)

add_executable(${ZP_TOOLS_TARGET})
target_sources(${ZP_TOOLS_TARGET}
        PRIVATE ${ZP_CORE_HEADERS}
        PRIVATE ${ZP_CORE_SRC}
        PRIVATE ${ZP_ENTRY_POINT}
        PRIVATE ${ZP_PLATFORM_HEADERS}
        PRIVATE ${ZP_PLATFORM_SRC}
        PRIVATE ${ZP_EXTERNAL_HEADERS}
        PRIVATE ${ZP_EXTERNAL_SRC}
        PRIVATE ${ZP_TOOLS_HEADERS}
        PRIVATE ${ZP_TOOLS_SRC}
)
target_include_directories(${ZP_TOOLS_TARGET}
        PRIVATE include
        PRIVATE external
)

set_target_properties(${ZP_TOOLS_TARGET} PROPERTIES
        VERSION ${ZP_VERSION}
)

target_include_directories(${ZP_TOOLS_TARGET}
        PRIVATE ${Vulkan_INCLUDE_DIRS}
)
target_link_options(${ZP_TOOLS_TARGET}
        PRIVATE -static-libgcc -static-libstdc++ -static
)
target_link_libraries(${ZP_TOOLS_TARGET}
        PRIVATE ${Vulkan_LIBRARIES}
        PRIVATE ${Vulkan_dxc_LIBRARY}
        PRIVATE "rpcrt4.lib"
        PRIVATE "dwmapi.dll"
        PRIVATE "Ws2_32.lib"
)

if (MSVC)
        set_target_properties(${ZP_TOOLS_TARGET} PROPERTIES
                LINK_FLAGS "/SUBSYSTEM:WINDOWS"
                VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )
        set_target_properties(${ZP_TOOLS_TARGET} PROPERTIES
                LINK_FLAGS "/SUBSYSTEM:WINDOWS"
                VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )
elseif (GCC OR CLANG)
else ()
endif ()

install(TARGETS ${ZP_TOOLS_TARGET}
        CONFIGURATIONS Debug
        RUNTIME DESTINATION ${ROOT_DIR}/Debug/bin
)
install(TARGETS ${ZP_TOOLS_TARGET}
        CONFIGURATIONS Release
        RUNTIME DESTINATION ${ROOT_DIR}/Release/bin
)
