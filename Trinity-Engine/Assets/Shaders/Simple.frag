#version 450

#include "ColorOutput.glsl"

layout(location = 0) in vec4 v_Color;
layout(location = 0) out vec4 o_Color;

layout(push_constant) uniform PushConstants
{
    mat4 u_ModelViewProjection;
    vec4 u_Color;
    uint u_ColorOutputTransfer;
} pc;

void main()
{
    o_Color = ApplySceneColorOutput(v_Color, pc.u_ColorOutputTransfer);
}