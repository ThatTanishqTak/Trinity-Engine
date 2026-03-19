#version 450

layout(set = 0, binding = 0) uniform sampler2D u_AlbedoTexture;

layout(location = 0) in vec3 v_WorldNormal;
layout(location = 1) in vec2 v_UV;
layout(location = 2) in vec3 v_WorldPosition;

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out vec4 o_Normal;
layout(location = 2) out vec4 o_Material;

layout(push_constant) uniform PushConstants
{
    mat4 u_ModelViewProjection;
    mat4 u_Model;
    vec4 u_Color; 
} pc;

void main()
{
    vec4 l_Albedo = texture(u_AlbedoTexture, v_UV) * pc.u_Color;
    vec3 l_N = normalize(v_WorldNormal);

    o_Albedo = l_Albedo;
    o_Normal = vec4(l_N * 0.5 + 0.5, 0.0);
    o_Material = vec4(0.5, 0.0, 0.0, 0.0);
}
