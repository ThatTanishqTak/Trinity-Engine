#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(push_constant) uniform inPushConstants
{
    mat4 MVP;
} pushConstants;

layout(location = 0) out vec3 outColor;

void main()
{
    gl_Position = pushConstants.MVP * vec4(inPosition, 1.0);
    outColor = inColor;
}