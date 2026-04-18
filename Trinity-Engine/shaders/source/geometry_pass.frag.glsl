#version 450

layout(location = 0) in vec3 v_FragPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out vec4 o_Normal;
layout(location = 2) out vec4 o_MRA;

void main()
{
    vec3 l_N = normalize(v_Normal);

    o_Albedo = vec4(0.8, 0.8, 0.8, 1.0);
    o_Normal = vec4(l_N * 0.5 + 0.5, 1.0);
    o_MRA    = vec4(0.0, 0.5, 1.0, 1.0);
}
