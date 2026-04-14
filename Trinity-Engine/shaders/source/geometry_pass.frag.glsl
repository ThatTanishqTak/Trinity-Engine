#version 450

layout(location = 0) in vec3 v_FragPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_Normal;

layout(set = 0, binding = 0) uniform sampler2D u_AlbedoMap;

void main()
{
    o_Color  = texture(u_AlbedoMap, v_TexCoord);
    o_Normal = vec4(normalize(v_Normal) * 0.5 + 0.5, 1.0);
}