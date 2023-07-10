#ifndef PACKED_VARYINGS_HLSL
#define PACKED_VARYINGS_HLSL

struct Varyings
{
#ifndef PACKED_VARYINGS_NO_POSITION_CS
    float4 positionCS : SV_POSITION;
#endif

#include "PackedVaryings/PackedVaryingsUV0.hlsl"
#include "PackedVaryings/PackedVaryingsColor.hlsl"

};

#endif // PACKED_VARYINGS_HLSL
