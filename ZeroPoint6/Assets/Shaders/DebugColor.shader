#pragma vertex DebugColorVertex
#pragma fragment DebugColorFragment

#pragma shader_feature_vertex _ USE_SOME_THING

#pragma target 6.0

#include "Common.hlsl"

#define ATTRIBUTES_USE_UV0
#define ATTRIBUTES_USE_COLOR
#include "Attributes.hlsl"

#define PACKED_VARYINGS_USE_COLOR
#include "PackedVaryings.hlsl"

Texture2D _Albedo;
SamplerState sampler_Albedo;

cbuffer PerFrame
{
    float4 _Time;
};

StructuredBuffer<uint> _Tiles;

float4x4 _ObjToWorld;
float4 _CCCC;

Varyings DebugColorVertex( in Attributes attr )
{
    Varyings v = (Varyings)0;
    v.positionCS = mul( _ObjToWorld, float4( attr.vertexOS, 1 ));
    v.color = attr.color * _Time.x + _Tiles[0];

    return v;
}

float4 DebugColorFragment( in Varyings v ) : SV_TARGET
{
    return v.color * _Albedo.Sample(sampler_Albedo, float2(0,0), 0) * _CCCC;
}
