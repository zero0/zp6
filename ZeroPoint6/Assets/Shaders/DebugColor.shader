#pragma vertex DebugColorVertex
#pragma fragment DebugColorFragment

#pragma enable_debug
#pragma shader_feature _ USE_SOME_THING
#pragma shader_feature_fragment _ FEATURE1 FEATURE2

#pragma invalid_shader_feature USE_SOME_THING FEATURE1

#pragma target 6.0

#include "Common.hlsl"

#define ATTRIBUTES_USE_UV0
#define ATTRIBUTES_USE_COLOR
#include "Attributes.hlsl"

#define PACKED_VARYINGS_USE_COLOR
#include "PackedVaryings.hlsl"

TEXTURE2D(_Albedo);
SAMPLER(_Albedo);

CBUFFER_START(PerFrame)
    float4 _Time;
CBUFFER_END

StructuredBuffer<uint> _Tiles;

float4x4 _ObjToWorld;
float4 _CCCC;

float4 GetTexture( TEXTURE2D_SAMPLER_PARAMS(albedo), float2 uv )
{
    return SAMPLE_TEXTURE2D(albedo, uv);
}

Varyings DebugColorVertex( in Attributes attr )
{
    ZERO_INITIALIZE(Varyings, v);

    v.positionCS = mul( _ObjToWorld, float4( attr.vertexOS, 1 ));
    v.color = attr.color * _Time.x + _Tiles[0];

    return v;
}

float4 DebugColorFragment( in Varyings v ) : SV_TARGET
{
    return v.color * GetTexture(TEXTURE2D_SAMPLER_ARGS(_Albedo), 0) * _CCCC;
}
