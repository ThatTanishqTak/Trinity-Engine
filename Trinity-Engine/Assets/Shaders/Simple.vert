#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV;

layout(push_constant) uniform PushConstants
{
    mat4 u_ModelViewProjection;
    vec4 u_Color;
    uint u_ColorInputTransfer;
    uint u_ColorOutputTransfer;
} pc;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec2 v_UV;

void main()
{
    gl_Position = pc.u_ModelViewProjection * vec4(a_Position, 1.0);
    v_Color = pc.u_Color;
    v_UV = a_UV;
}
