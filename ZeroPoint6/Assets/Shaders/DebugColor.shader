
#include "Common.hlsl"

#define ATTRIBUTES_USE_UV0
#define ATTRIBUTES_USE_COLOR
#include "Attributes.hlsl"

#define PACKED_VARYINGS_USE_COLOR
#include "PackedVaryings.hlsl"

Varyings DebugColorVertex( in Attributes attr )
{
    Varyings v = (Varyings)0;
    v.positionCS = float4( attr.vertexOS, 1 );
    v.color = attr.color;

    return v;
}

float4 DebugColorFragment( in Varyings v ) : SV_TARGET
{
    return v.color;
}
