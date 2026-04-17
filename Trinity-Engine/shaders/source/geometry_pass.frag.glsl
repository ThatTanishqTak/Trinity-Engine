#version 450

layout(location = 0) in vec3 v_FragPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_Normal;

void main()
{
    vec3 l_N = normalize(v_Normal);
    o_Color  = vec4(abs(l_N), 1.0);
    o_Normal = vec4(l_N * 0.5 + 0.5, 1.0);
}