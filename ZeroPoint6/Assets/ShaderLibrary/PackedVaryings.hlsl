#ifndef PACKED_VARYINGS_HLSL
#define PACKED_VARYINGS_HLSL

struct Varyings
{
#ifndef PACKED_VARYINGS_NO_POSITION_CS
    float4 positionCS : SV_POSITION;
#endif

#include "PackedVaryings/PackedVaryingsUV0.hlsl"
#include "PackedVaryings/PackedVaryingsColor.hlsl"

#ifdef PACKED_VARYINGS_USE_IS_FRONT_FACE
    bool isFrontFace : IS_FRONT_FACE_SEMANTIC;
#endif // PACKED_VARYINGS_USE_IS_FRONT_FACE

#ifdef PACKED_VARYINGS_USE_PRIMITIVE_ID
    uint primitiveID : PRIMITIVE_ID_SEMANTIC;
#endif // PACKED_VARYINGS_USE_PRIMITIVE_ID

#ifdef PACKED_VARYINGS_USE_SAMPLE_INDEX
    uint sampleIndex : SAMPLE_INDEX_SEMANTIC;
#endif // PACKED_VARYINGS_USE_SAMPLE_INDEX
};

#endif // PACKED_VARYINGS_HLSL
