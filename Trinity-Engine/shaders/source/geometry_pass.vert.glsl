#version 450

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_UV;

layout(push_constant) uniform PushBlock
{
    mat4 Model;
    mat4 ViewProjection;
} u_Push;

layout(location = 0) out vec3 v_WorldPosition;
layout(location = 1) out vec3 v_WorldNormal;
layout(location = 2) out vec2 v_UV;

void main()
{
    vec4 l_WorldPosition = u_Push.Model * vec4(in_Position, 1.0);
    v_WorldPosition = l_WorldPosition.xyz;

    mat3 l_NormalMatrix = mat3(u_Push.Model);
    v_WorldNormal = normalize(l_NormalMatrix * in_Normal);

    v_UV = in_UV;

    gl_Position = u_Push.ViewProjection * l_WorldPosition;
}
