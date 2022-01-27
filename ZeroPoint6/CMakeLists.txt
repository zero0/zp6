
set(ZP_VERSION_MAJOR 6)
set(ZP_VERSION_MINOR 0)
set(ZP_VERSION_PATCH 0)
set(ZP_VERSION ${ZP_VERSION_MAJOR}.${ZP_VERSION_MINOR}.${ZP_VERSION_PATCH})

project(ZeroPoint VERSION ${ZP_VERSION} LANGUAGES CXX)

set(ZP_TARGET ${PROJECT_NAME}_${PLATFORM_NAME}_${PLATFORM_ARCH})

set(ZP_RENDERING_API Vulkan)
#find_package(Vulkan REQUIRED)

# Source Files
set(ZP_CORE_SRC
        "src/Core/Common.cpp"
        )

set(ZP_PLATFORM_SRC
        "src/Platform/${PLATFORM_FULL_NAME}/${PLATFORM_FULL_NAME}Main.cpp"
        "src/Platform/${PLATFORM_FULL_NAME}/${PLATFORM_FULL_NAME}Platform.cpp"
        )

set(ZP_RENDERING_SRC
        "src/Rendering/${ZP_RENDERING_API}/${ZP_RENDERING_API}RenderingEngine.cpp"
        "src/Rendering/${ZP_RENDERING_API}/${ZP_RENDERING_API}Shader.cpp"
        )

set(ZP_ENGINE_SRC
        "src/Core/Allocator.cpp"
        "src/Engine/Component.cpp"
        "src/Engine/Engine.cpp"
        "src/Engine/Entity.cpp"
        "src/Engine/JobSystem.cpp"
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
        "include/Core/Common.h"
        "include/Core/Defines.h"
        "include/Core/Macros.h"
        "include/Core/Types.h"
        "include/Core/Vector.h"
        "include/Core/Allocator.h"
        )

set(ZP_ENGINE_HEADERS
        "include/Engine/ApplicationEntryPoint.h"
        "include/Engine/BaseComponentSystem.h"
        "include/Engine/Component.h"
        "include/Engine/ComponentSignature.h"
        "include/Engine/Engine.h"
        "include/Engine/EngineEntryPointAPI.h"
        "include/Engine/Entity.h"
        "include/Engine/JobSystem.h"
        "include/Engine/MemoryLabels.h"
        )

set(ZP_PLATFORM_HEADERS
        "include/Platform/Platform.h"
        )

set(ZP_RENDERING_HEADERS
        "include/Rendering/RenderingEngine.h"
        "include/Rendering/Shader.h"
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

add_executable(${ZP_TARGET})
target_sources(${ZP_TARGET}
        PRIVATE ${ZP_HEADERS}
        PRIVATE ${ZP_SRC}
        )
target_include_directories(${ZP_TARGET}
        PRIVATE include
        )

# add_module(CoreModule ${ZP_CORE_MODULE})
# add_module(PlatformModule ${ZP_PLATFORM_MODULE})

target_link_libraries(${ZP_TARGET}
        #PRIVATE CoreModule
        #PRIVATE PlatformModule
        )

#if (${ZP_RENDERING_API} EQUAL Vulkan)
cmake_path(SET VulkanSDK $ENV{VULKAN_SDK})

target_include_directories(${ZP_TARGET}
        PRIVATE "${VulkanSDK}/Include"
        )
target_link_libraries(${ZP_TARGET}
        PRIVATE "${VulkanSDK}/Lib/vulkan-1.lib"
        PRIVATE "${VulkanSDK}/Lib/VkLayer_utils.lib"
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
