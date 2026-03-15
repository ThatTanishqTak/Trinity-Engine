#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV;

layout(push_constant) uniform PushConstants
{
    mat4 u_ModelViewProjection;
    mat4 u_Model;
} pc;

layout(location = 0) out vec3 v_WorldNormal;
layout(location = 1) out vec2 v_UV;
layout(location = 2) out vec3 v_WorldPosition;

void main()
{
    vec4 l_WorldPos = pc.u_Model * vec4(a_Position, 1.0);
    gl_Position = pc.u_ModelViewProjection * vec4(a_Position, 1.0);
    v_WorldPosition = l_WorldPos.xyz;
    v_WorldNormal = normalize(mat3(transpose(inverse(pc.u_Model))) * a_Normal);
    v_UV = a_UV;
}
