#ifndef COMMON_HLSL
#define COMMON_HLSL

#if SHADER_API_D3D
#include "API/D3D.hlsl"
#elif SHADER_API_VULKAN
#include "API/Vulkan.hlsl"
#else
#error "Unknown Shader API"
#endif

#define ZERO_INITIALIZE( t, v )         t v = (t)0

#endif // COMMON_HLSL
