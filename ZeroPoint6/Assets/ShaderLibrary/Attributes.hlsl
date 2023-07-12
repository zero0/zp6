#ifndef ATTRIBUTES_HLSL
#define ATTRIBUTES_HLSL

#ifndef ATTRIBUTE_POSITION_TYPE
#define ATTRIBUTE_POSITION_TYPE float3
#endif // ATTRIBUTE_POSITION_TYPE

#ifndef ATTRIBUTE_COLOR_TYPE
#define ATTRIBUTE_COLOR_TYPE float4
#endif // ATTRIBUTE_COLOR_TYPE

#ifndef ATTRIBUTE_UV0_TYPE
#define ATTRIBUTE_UV0_TYPE float2
#endif // ATTRIBUTE_UV0_TYPE

#ifndef ATTRIBUTE_UV1_TYPE
#define ATTRIBUTE_UV1_TYPE float2
#endif // ATTRIBUTE_UV1_TYPE

#ifndef ATTRIBUTE_NORMAL_TYPE
#define ATTRIBUTE_NORMAL_TYPE float3
#endif // ATTRIBUTE_NORMAL_TYPE

#ifndef ATTRIBUTE_TANGENT_TYPE
#define ATTRIBUTE_TANGENT_TYPE float4
#endif // ATTRIBUTE_TANGENT_TYPE

struct Attributes
{
#ifndef ATTRIBUTES_NO_VERTEX_OS
    ATTRIBUTE_POSITION_TYPE vertexOS : POSITION;
#endif // ATTRIBUTES_NO_VERTEX_OS

#ifdef ATTRIBUTES_USE_COLOR
    ATTRIBUTE_COLOR_TYPE color : COLOR;
#endif // ATTRIBUTES_USE_COLOR

#ifdef ATTRIBUTES_USE_UV0
    ATTRIBUTE_UV0_TYPE uv0 : TEXCOORD0;
#endif // ATTRIBUTES_USE_UV0

#ifdef ATTRIBUTES_USE_UV1
    ATTRIBUTE_UV1_TYPE uv1 : TEXCOORD1;
#endif // ATTRIBUTES_USE_UV1

#ifdef ATTRIBUTES_USE_NORMAL
    ATTRIBUTE_NORMAL_TYPE normal : NORMAL;
#endif // ATTRIBUTES_USE_NORMAL

#ifdef ATTRIBUTES_USE_TANGENT
    ATTRIBUTE_TANGENT_TYPE tangent : TANGENT;
#endif // ATTRIBUTES_USE_TANGENT
};

#endif // ATTRIBUTES_HLSL