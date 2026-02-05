#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV;
layout(location = 3) in vec4 a_Tangent;

layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 Proj;
    mat4 ViewProj;
} u_Global;

layout(push_constant) uniform ObjectPush
{
    mat4 Model;
} u_Object;

void main()
{
    gl_Position = u_Global.ViewProj * u_Object.Model * vec4(a_Position, 1.0);
}