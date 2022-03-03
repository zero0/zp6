#version 450

layout(location = 0) in vec2 inUV0;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outFragColor;

layout(binding = 3) uniform sampler2D _MainTex;

void main()
{
    vec4 color = inColor * texture(_MainTex, inUV0);
    outFragColor = color;
}
