
struct Attributes
{
    float3 positionOS : POSITION;
};

struct Varyings
{
    float4 positionSS : SV_POSITION;
    float4 color : TEXCOODR0;
};

Varyings LitVertex( in Attributes attr )
{
    Varyings v = (Varyings)0;
    v.positionSS = float4( attr.positionOS, 1 );
    v.color = float4(1,1,1,1);

    return v;
}

float4 LitFragment( in Varyings varyings ) : SV_TARGET
{
    return varyings.color;
}
