
set(ZP_VERSION_MAJOR 6)
set(ZP_VERSION_MINOR 0)
set(ZP_VERSION_PATCH 0)
set(ZP_VERSION ${ZP_VERSION_MAJOR}.${ZP_VERSION_MINOR}.${ZP_VERSION_PATCH})

find_package(Vulkan REQUIRED COMPONENTS volk dxc)

project(ZeroPoint VERSION ${ZP_VERSION} LANGUAGES CXX)

set(ZP_TARGET ${PROJECT_NAME}_${PLATFORM_NAME}_${PLATFORM_ARCH})

set(ZP_RENDERING_API Vulkan)

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
    "src/Rendering/GraphicsCommandBuffer.cpp"
    "src/Rendering/GraphicsResource.cpp"
    "src/Rendering/ImmediateModeRenderer.cpp"
    #    "src/Rendering/RenderGraph.cpp"
    "src/Rendering/RenderSystem.cpp"
)

set(ZP_RENDERING_API_SRC
    "src/Rendering/${ZP_RENDERING_API}/${ZP_RENDERING_API}GraphicsDevice.cpp"
    #"src/Rendering/${ZP_RENDERING_API}/${ZP_RENDERING_API}RenderingEngine.cpp"
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
    "include/Core/Hash.h"
    "include/Core/Job.h"
    "include/Core/Log.h"
    "include/Core/Macros.h"
    "include/Core/Map.h"
    "include/Core/Math.h"
    "include/Core/Memory.h"
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
    "include/Rendering/GraphicsCommandBuffer.h"
    "include/Rendering/GraphicsDevice.h"
    "include/Rendering/GraphicsResource.h"
    "include/Rendering/GraphicsTypes.h"
    "include/Rendering/ImmediateModeRenderer.h"
    "include/Rendering/Material.h"
    #    "include/Rendering/RenderGraph.h"
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
#     "modules/Core/Core.ixx"
#     "modules/Core/Core-Types.ixx"
#     "modules/Core/Core-Rect.ixx"
#     "modules/Core/Core-Vector2.ixx"
#     "modules/Core/Core-Printf.ixx"
#     )

# set(ZP_PLATFORM_MODULE
#     "modules/Platform/Platform.ixx"
#     "src/Platform/Platform-${PLATFORM_FULL_NAME}.cpp"
#     )

set(ZP_EXTERNAL_HEADERS
    "external/tlsf/tlsf.h"
)
source_group(External Header FILES ${ZP_EXTERNAL_HEADERS})

set(ZP_EXTERNAL_SRC
    "external/tlsf/tlsf.c"
    "${Vulkan_INCLUDE_DIRS}/Volk/volk.c"
)
source_group(External Source FILES ${ZP_EXTERNAL_SRC})

#if (GCC OR CLANG)
#target_compile_definitions(${ZP_TARGET}}
#    PUBLIC -D_WIN32_WINNT=0x0A00
#    )
#endif ()

add_executable(${ZP_TARGET}
    "res/${PLATFORM_FULL_NAME}/resource.rc")
target_sources(${ZP_TARGET} PRIVATE
    ${ZP_HEADERS}
    ${ZP_SRC}
    ${ZP_EXTERNAL_HEADERS}
    ${ZP_EXTERNAL_SRC}
)
target_include_directories(${ZP_TARGET} PRIVATE
    include
    external
)

# add_module(CoreModule ${ZP_CORE_MODULE})
# add_module(PlatformModule ${ZP_PLATFORM_MODULE})

# target_link_libraries(${ZP_TARGET}
#     PRIVATE CoreModule
#     PRIVATE PlatformModule
#     )

#if (${ZP_RENDERING_API} EQUAL Vulkan)
#cmake_path(SET VulkanSDK $ENV{VULKAN_SDK})

get_filename_component(_Vulkan_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)

message(CHECK_START "Finding volk")
if(Vulkan_volk_FOUND)
    message(CHECK_PASS "Found volk ${Vulkan_volk_LIBRARY}")
else()
    message(CHECK_FAIL "Unable to find volk")
endif()

# Define VK_NO_PROTOTYPES to avoid including Vulkan prototypes
# This is necessary because we are using volk to load Vulkan functions
target_compile_definitions(${ZP_TARGET} PRIVATE VK_NO_PROTOTYPES)

if(${PLATFORM_NAME} STREQUAL "Win")
    target_compile_definitions(${ZP_TARGET} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()

find_program(SLANGC_EXE
    NAMES slangc
    HINTS ${Vulkan_LIB_DIR}/../Bin
)

message(CHECK_START "Finding shader compilers")
if(SLANGC_EXE)
    message(CHECK_PASS "Found slangc: ${SLANGC_EXE}")

    set(_SLANG_FLAGS
        -I Assets/ShaderLibrary/
        -profile sm_6_6+spirv_1_6
        -capability spvInt64Atomics+spvShaderInvocationReorderNV+spvShaderClockKHR+spvRayTracingMotionBlurNV
        -target spirv
        -emit-spirv-directly
        -force-glsl-scalar-layout
        -fvk-use-entrypoint-name
        -g3
        -source-embed-style text
        )

    # Compile Slang shaders using slangc
    file(GLOB_RECURSE SHADER_SLANG_FILES LIST_DIRECTORIES FALSE "Assets/Shaders/*.slang" "Assets/ShaderLibrary/*.slang")
    cmake_path(APPEND SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} generated Assets)
    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
    foreach(SHADER ${SHADER_SLANG_FILES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        string(REPLACE "." "_" EMBED_SHADER_NAME ${SHADER_NAME})
        cmake_path(APPEND OUTPUT_FILE ${SHADER_OUTPUT_DIR} Shaders ${SHADER_NAME}.h)
        get_filename_component(OUTPUT_NAME ${OUTPUT_FILE} NAME)
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${SLANGC_EXE} ${_SLANG_FLAGS} -source-embed-name ${EMBED_SHADER_NAME} -o ${OUTPUT_FILE} ${SHADER}
            MAIN_DEPENDENCY ${SHADER}
            COMMENT "slangc: Compiling shader ${SHADER_NAME} -> ${OUTPUT_NAME}..."
            VERBATIM
        )
        list(APPEND SHADER_HEADERS ${OUTPUT_FILE})
    endforeach()
else ()
    message(CHECK_FAIL "Unable to find slangc")
endif()

target_sources(${ZP_TARGET} PRIVATE ${SHADER_HEADERS})
target_include_directories(${ZP_TARGET} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/")

target_include_directories(${ZP_TARGET}
    PRIVATE ${Vulkan_INCLUDE_DIRS}
)
target_link_directories(${ZP_TARGET}
    PRIVATE ${_Vulkan_LIB_DIR}
)
target_link_options(${ZP_TARGET}
    PRIVATE -static-libgcc -static-libstdc++ -static
)
target_link_libraries(${ZP_TARGET} PRIVATE
    rpcrt4
    dwmapi
    Ws2_32
    dbghelp
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
target_link_libraries(${ZP_TOOLS_TARGET} PRIVATE
    ${Vulkan_LIBRARIES}
    ${Vulkan_dxc_LIBRARY}
    rpcrt4.lib
    dwmapi.dll
    Ws2_32.lib
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
