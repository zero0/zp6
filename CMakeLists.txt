cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(zp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_SYSROOT_FLAG_CODE "list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS ixx)")
set(USE_MODULES no)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(GCC 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CLANG 1)
endif()

if (GCC OR CLANG)
    set(CMAKE_CXX_FLAGS "-fms-extensions")
endif()

if (USE_MODULES)
if (GCC OR CLANG)
    set(CMAKE_CXX_FLAGS "-fmodules-ts")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "/experimental:module")
endif()

function(add_module name)
    cmake_path(SET bin_dir ${CMAKE_BINARY_DIR}/modules/${name})
    set(sources)

    if (GCC)
        # GCC doesn't understand how to compile .ixx files
        foreach(arg IN LISTS ARGN)
            if (arg MATCHES .*\\.ixx)
                get_filename_component(rel_dir ${arg} DIRECTORY)
                get_filename_component(file_name ${arg} NAME_WLE)
                cmake_path(SET final_file_directory ${bin_dir}/${rel_dir})
                cmake_path(SET final_file_path ${final_file_directory}/${file_name}.cpp)
                file(MAKE_DIRECTORY ${final_file_directory})
                file(COPY_FILE ${arg} ${final_file_path} ONLY_IF_DIFFERENT)
                list(APPEND sources ${final_file_path})
            endif()
        endforeach()
    elseif(MSVC)
        # MSVC only understands .ixx files when building c++20 modules
        set(sources ${ARGN})
    else()
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()

    add_library(${name} STATIC ${sources})
endfunction()
endif()

set(PLATFORM_FULL_NAME Windows)
set(PLATFORM_NAME Win)
set(PLATFORM_ARCH x64)

set(ROOT_DIR ${PROJECT_SOURCE_DIR})

add_subdirectory(ZeroPoint6)
#add_subdirectory(ShadowGame)
