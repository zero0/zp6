#pragma vertex DebugColorVertex
#pragma fragment DebugColorFragment

#pragma enable_debug
#pragma shader_feature _ USE_SOMETHING_COOL
#pragma shader_feature_fragment _ FEATURE1 FEATURE2 _FEATURE3

#pragma invalid_shader_feature USE_SOME_THING FEATURE1

#pragma target 6.0

#include "Common.hlsl"

#define ATTRIBUTES_USE_UV0
#define ATTRIBUTES_USE_UV1
#define ATTRIBUTES_USE_COLOR
#define ATTRIBUTES_USE_NORMAL
#include "Attributes.hlsl"

#define PACKED_VARYINGS_USE_COLOR
#define PACKED_VARYINGS_USE_UV0
#include "PackedVaryings.hlsl"

TEXTURE2D(_Albedo);
SAMPLER(_Albedo);

CBUFFER_START(PerFrame)
    float4 _Time;
    float4 _SomethingElse;
CBUFFER_END

CBUFFER_START(PerCamera)
    float4x4 _WorldToHClip;
    float4 _ScreenSize;
CBUFFER_END

CBUFFER_START(PerObject)
    float4x4 _ObjToWorld;
    float4x4 _WorldToObject;
CBUFFER_END

CBUFFER_START(PerMaterial)
    float4 _CCCC;
CBUFFER_END

struct TileData
{
    uint2 data;
    float2 other;
};

StructuredBuffer<TileData> _Tiles;


float4 GetTexture( TEXTURE2D_SAMPLER_PARAMS(albedo), float2 uv )
{
    return SAMPLE_TEXTURE2D(albedo, uv);
}

Varyings DebugColorVertex( in Attributes attr )
{
    ZERO_INITIALIZE(Varyings, v);

    v.positionCS = mul( _ObjToWorld, float4( attr.vertexOS, 1 ));
    v.color = attr.color * _Time.x + _Tiles[0].data.x;
    v.uv0 = attr.uv0;

    return v;
}

float4 DebugColorFragment( in Varyings v ) : SV_TARGET
{
    return v.color * GetTexture(TEXTURE2D_SAMPLER_ARGS(_Albedo), v.uv0) * _CCCC * _ScreenSize;
}
