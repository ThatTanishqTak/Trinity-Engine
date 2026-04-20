#version 450

layout(set = 0, binding = 0) uniform sampler2D u_AlbedoTexture;

layout(location = 0) in vec3 v_FragPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out vec4 o_Normal;
layout(location = 2) out vec4 o_MRA;

void main()
{
    vec3 l_N = normalize(v_Normal);

    o_Albedo = texture(u_AlbedoTexture, v_TexCoord);
    o_Normal = vec4(l_N * 0.5 + 0.5, 1.0);
    o_MRA    = vec4(0.0, 0.5, 1.0, 1.0);
}
