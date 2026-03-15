#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV;

layout(push_constant) uniform PushConstants
{
    mat4 u_LightSpaceModelViewProjection;
} pc;

void main()
{
    gl_Position = pc.u_LightSpaceModelViewProjection * vec4(a_Position, 1.0);
}
