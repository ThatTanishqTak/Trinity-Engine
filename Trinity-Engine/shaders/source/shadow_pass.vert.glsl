#version 450

layout(location = 0) in vec3 a_Position;

layout(push_constant) uniform PushConstants
{
    mat4 u_LightSpaceMatrix;
    mat4 u_Model;
} u_Push;

void main()
{
    gl_Position = u_Push.u_LightSpaceMatrix * u_Push.u_Model * vec4(a_Position, 1.0);
}
