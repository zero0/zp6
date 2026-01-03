
#define ATTRIBUTES_USE_UV0
//#define ATTRIBUTES_USE_UV1
#define ATTRIBUTES_USE_COLOR
#define ATTRIBUTES_USE_NORMAL
#include "../ShaderLibrary/Attributes.hlsl"

#define PACKED_VARYINGS_USE_COLOR
#define PACKED_VARYINGS_USE_UV0
#include "../ShaderLibrary/PackedVaryings.hlsl"

namespace zp
{

}

[shader("vertex")]
void Vert(in Attributes v, out Varyings o)
{
    o.color = v.color;
}

[shader("pixel")]
void Frag(in Varyings v, out float4 pixel)
{
    pixel = v.color;
}
