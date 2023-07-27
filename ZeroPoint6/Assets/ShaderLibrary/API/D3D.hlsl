#ifndef D3D_HLSL
#define D3D_HLSL

#define INSTANCE_ID_SEMANTIC                                            SV_InstanceID
#define VERTEX_ID_SEMANTIC                                              SV_VertexID
#define PRIMITIVE_ID_SEMANTIC                                           SV_PrimitiveID
#define SAMPLE_INDEX_SEMANTIC                                           SV_SampleIndex
#define IS_FRONT_FACE_SEMANTIC                                          SV_IsFrontFace

#define VFACE_PARAM(f)                                                  float face##f : VFACE
#define VFACE_ARG(f)                                                    face##f
#define IS_FRONT_FACE(f)                                                (bool)( VFACE_ARG(f) > 0 )

#define CBUFFER_START(n)                                                cbuffer n {
#define CBUFFER_START_REGISTER(n, r)                                    cbuffer n : register( b##r ) {
#define CBUFFER_END                                                     }

#define PACKED_OFFSET(o)                                                : packedoffset( c##o )
#define PACKED_OFFSET_ELEMENTS(o, e)                                    : packedoffset( c##o##.##e )

#define TEXTURE2D_T(n, t)                                               Texture2D<t> n
#define TEXTURE2D_T_REGISTER(n, t, r)                                   Texture2D<t> n : register( t##r )

#define TEXTURE2D(n)                                                    Texture2D n
#define TEXTURE2D_REGISTER(n, r)                                        Texture2D n : register( t##r )

#define SAMPLER(n)                                                      SamplerState sampler##n
#define SAMPLER_REGISTER(n, r)                                          SamplerState sampler##n : register( s##r )

#define SAMPLER_CMP(n)                                                  SamplerComparisonState samplerCmp##n
#define SAMPLER_CMP_REGISTER(n, r)                                      SamplerComparisonState samplerCmp##n : register( s##r )

#define TEXTURE2D_SAMPLER_PARAMS(n)                                     Texture2D n, SamplerState sampler##n
#define TEXTURE2D_SAMPLER_CMP_PARAMS(n)                                 Texture2D n, SamplerComparisonState samplerCmp##n

#define TEXTURE2D_SAMPLER_ARGS(n)                                       n, sampler##n
#define TEXTURE2D_SAMPLER_CMP_ARGS(n)                                   n, samplerCmp##n

#define SAMPLE_TEXTURE2D(n, uv)                                         n.Sample( sampler##n, (uv).xy, 0 )
#define SAMPLE_TEXTURE2D_OFFSET(n, uv, o)                               n.Sample( sampler##n, (uv).xy, (o).xy )

#define SAMPLE_BIAS_TEXTURE2D(n, uv, b)                                 n.SampleBias( sampler##n, (uv).xy, (b), 0 )
#define SAMPLE_BIAS_TEXTURE2D_OFFSET(n, uv, b, o)                       n.SampleBias( sampler##n, (uv).xy, (b), (o).xy )

#define SAMPLE_CMP_TEXTURE2D(n, uv, c)                                  n.SampleCmp( samplerCmp##n, (uv).xy, (c), 0 )
#define SAMPLE_CMP_TEXTURE2D_OFFSET(n, uv, c, o)                        n.SampleCmp( samplerCmp##n, (uv).xy, (c), (o).xy )

#define SAMPLE_CMPLEVELZERO_TEXTURE2D(n, uv, c)                         n.SampleCmpLevelZero( samplerCmp##n, (uv).xy, (c), 0 )
#define SAMPLE_CMPLEVELZERO_TEXTURE2D_OFFSET(n, uv, c, o)               n.SampleCmpLevelZero( samplerCmp##n, (uv).xy, (c), (o).xy )

#define SAMPLE_GRAD_TEXTURE2D(n, uv, dx, dy)                            n.SampleGrad( sampler##n, (uv).xy, (dx).xy, (dy).xy, 0 )
#define SAMPLE_GRAD_TEXTURE2D_OFFSET(n, uv, dx, dy, o)                  n.SampleGrad( sampler##n, (uv).xy, (dx).xy, (dy).xy, (o).xy )

#define SAMPLE_LEVEL_TEXTURE2D(n, uv, l)                                n.SampleLevel( sampler##n, (uv).xy, (l), 0 )
#define SAMPLE_LEVEL_TEXTURE2D_OFFSET(n, uv, l, o)                      n.SampleLevel( sampler##n, (uv).xy, (l), (o).xy )

#define LOAD_TEXTURE2D(n, loc)                                          n.Load( int3( (loc).xy, 0 ), 0 )
#define LOAD_TEXTURE2D_MIP(n, loc, m)                                   n.Load( int3( (loc).xy, (m) ), 0 )
#define LOAD_TEXTURE2D_OFFSET(n, loc, o)                                n.Load( int3( (loc).xy, 0 ), (o).xy )
#define LOAD_TEXTURE2D_MIP_OFFSET(n, loc, m, o)                         n.Load( int3( (loc).xy, (m) ), (o).xy )

#define GATHER_TEXTURE2D(n, uv)                                         n.Gather( sampler##n, (uv).xy, 0 )
#define GATHER_TEXTURE2D_OFFSET(n, uv, o)                               n.Gather( sampler##n, (uv).xy, (o).xy )

#define GATHER_CMP_TEXTURE2D(n, uv, c)                                  n.GatherCmp( sampler##n, (uv).xy, (c), 0 )
#define GATHER_CMP_TEXTURE2D_OFFSET(n, uv, c, o)                        n.GatherCmp( sampler##n, (uv).xy, (c), (o).xy )

#define GATHER_RED_TEXTURE2D(n, uv)                                     n.GatherRed( sampler##n, (uv).xy, 0 )
#define GATHER_RED_TEXTURE2D_OFFSET(n, uv, o)                           n.GatherRed( sampler##n, (uv).xy, (o).xy )
#define GATHER_RED_TEXTURE2D_OFFSET4(n, uv, o0, o1, o2, o3)             n.GatherRed( sampler##n, (uv).xy, (o0).xy, (o1).xy, (o2).xy, (o3).xy )

#define GATHER_GREEN_TEXTURE2D(n, uv)                                   n.GatherGreen( sampler##n, (uv).xy, 0 )
#define GATHER_GREEN_TEXTURE2D_OFFSET(n, uv, o)                         n.GatherGreen( sampler##n, (uv).xy, (o).xy )
#define GATHER_GREEN_TEXTURE2D_OFFSET4(n, uv, o0, o1, o2, o3)           n.GatherGreen( sampler##n, (uv).xy, (o0).xy, (o1).xy, (o2).xy, (o3).xy )

#define GATHER_BLUE_TEXTURE2D(n, uv)                                    n.GatherBlue( sampler##n, (uv).xy, 0 )
#define GATHER_BLUE_TEXTURE2D_OFFSET(n, uv, o)                          n.GatherBlue( sampler##n, (uv).xy, (o).xy )
#define GATHER_BLUE_TEXTURE2D_OFFSET4(n, uv, o0, o1, o2, o3)            n.GatherBlue( sampler##n, (uv).xy, (o0).xy, (o1).xy, (o2).xy, (o3).xy )

#define GATHER_ALPHA_TEXTURE2D(n, uv)                                   n.GatherAlpha( sampler##n, (uv).xy, 0 )
#define GATHER_ALPHA_TEXTURE2D_OFFSET(n, uv, o)                         n.GatherAlpha( sampler##n, (uv).xy, (o).xy )
#define GATHER_ALPHA_TEXTURE2D_OFFSET4(n, uv, o0, o1, o2, o3)           n.GatherAlpha( sampler##n, (uv).xy, (o0).xy, (o1).xy, (o2).xy, (o3).xy )

#define GATHER_CMP_RED_TEXTURE2D(n, uv, c)                              n.GatherRed( samplerCmp##n, (uv).xy, (c), 0 )
#define GATHER_CMP_RED_TEXTURE2D_OFFSET(n, uv, c, o)                    n.GatherRed( samplerCmp##n, (uv).xy, (c), (o).xy )
#define GATHER_CMP_RED_TEXTURE2D_OFFSET4(n, uv, c, o0, o1, o2, o3)      n.GatherRed( samplerCmp##n, (uv).xy, (c), (o0).xy, (o1).xy, (o2).xy, (o3).xy )

#define GATHER_CMP_GREEN_TEXTURE2D(n, uv, c)                            n.GatherGreen( samplerCmp##n, (uv).xy, (c), 0 )
#define GATHER_CMP_GREEN_TEXTURE2D_OFFSET(n, uv, c, o)                  n.GatherGreen( samplerCmp##n, (uv).xy, (c), (o).xy )
#define GATHER_CMP_GREEN_TEXTURE2D_OFFSET4(n, uv, c, o0, o1, o2, o3)    n.GatherGreen( samplerCmp##n, (uv).xy, (c), (o0).xy, (o1).xy, (o2).xy, (o3).xy )

#define GATHER_CMP_BLUE_TEXTURE2D(n, uv, c)                             n.GatherBlue( samplerCmp##n, (uv).xy, (c), 0 )
#define GATHER_CMP_BLUE_TEXTURE2D_OFFSET(n, uv, c, o)                   n.GatherBlue( samplerCmp##n, (uv).xy, (c), (o).xy )
#define GATHER_CMP_BLUE_TEXTURE2D_OFFSET4(n, uv, c, o0, o1, o2, o3)     n.GatherBlue( samplerCmp##n, (uv).xy, (c), (o0).xy, (o1).xy, (o2).xy, (o3).xy )

#define GATHER_CMP_ALPHA_TEXTURE2D(n, uv, c)                            n.GatherAlpha( samplerCmp##n, (uv).xy, (c), 0 )
#define GATHER_CMP_ALPHA_TEXTURE2D_OFFSET(n, uv, c, o)                  n.GatherAlpha( samplerCmp##n, (uv).xy, (c), (o).xy )
#define GATHER_CMP_ALPHA_TEXTURE2D_OFFSET4(n, uv, c, o0, o1, o2, o3)    n.GatherAlpha( samplerCmp##n, (uv).xy, (c), (o0).xy, (o1).xy, (o2).xy, (o3).xy )


#endif // D3D_HLSL
