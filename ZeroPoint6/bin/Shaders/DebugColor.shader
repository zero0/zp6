
#include "Common.hlsl"

struct Attributes
{
    float3 positionOS : POSITION;
    float2 uv0 : TEXCOORD0;
    float4 color : COLOR;
};

#define PACKED_VARYINGS_USE_COLOR
#include "PackedVaryings.hlsl"

cbuffer PerMaterial
{
    float4 _Color;
};

Varyings DebugColorVertex( in Attributes attr )
{
    Varyings v = (Varyings)0;
    v.positionCS = float4( attr.positionOS, 1 );
    v.color = attr.color;

    return v;
}

float4 DebugColorFragment( in Varyings varyings ) : SV_TARGET
{
    return varyings.color * _Color;
}
