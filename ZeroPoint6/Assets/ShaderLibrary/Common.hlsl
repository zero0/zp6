#ifndef COMMON_HLSL
#define COMMON_HLSL

#if SHADER_PLATFORM_D3D
#include "Platform/D3D.hlsl"
#elif SHADER_PLATFORM_VULKAN
#else
#error "Unknown Shader Platform"
#endif

#endif // COMMON_HLSL
