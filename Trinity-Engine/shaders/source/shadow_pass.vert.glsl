#version 450

layout(location = 0) in vec3 in_Position;

layout(push_constant) uniform PushBlock
{
    mat4 Model;
    mat4 LightViewProjection;
} u_Push;

void main()
{
    gl_Position = u_Push.LightViewProjection * u_Push.Model * vec4(in_Position, 1.0);
}