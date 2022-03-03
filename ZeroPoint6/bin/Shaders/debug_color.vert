#version 450 core

layout(location = 0) in vec3 inVertexOS;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 outColor;

/*
layout(binding = 0) uniform PerFrame
{
    mat4 viewProjection;
    mat4 invViewProjection;
    vec4 time;
    vec4 resolution;
} perFrame;

layout(binding = 1) uniform PerMaterial
{
    vec4 _Color;
} perMaterial;

layout(binding = 2) uniform PerDraw
{
    mat4 localToWorld;
    mat4 worldToLocal;
} perDraw;

layout(push_constant) uniform Constants
{
    vec4 gColor;
} constants;
*/

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    outColor = inColor;
    gl_Position = vec4(inVertexOS, 1);
}

/*
void old_main()
{
    outColor = perMaterial._Color * inColor;

    vec4 positionWS = perDraw.localToWorld * vec4(inVertexOS, 1.0);
    gl_Position = perFrame.viewProjection * positionWS;
}
*/
