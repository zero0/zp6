#ifndef COMMON_HLSL
#define COMMON_HLSL

#if SHADER_API_D3D
#include "Platform/D3D.hlsl"
#elif SHADER_API_VULKAN
#include "Platform/Vulkan.hlsl"
#else
#error "Unknown Shader Platform"
#endif

#endif // COMMON_HLSL
