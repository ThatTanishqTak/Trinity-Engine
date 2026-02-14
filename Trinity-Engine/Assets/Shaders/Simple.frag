#version 450

#include "ColorOutput.glsl"

layout(location = 0) in vec4 v_Color;
layout(location = 0) out vec4 o_Color;

layout(push_constant) uniform PushConstants
{
    mat4 u_ModelViewProjection;
    vec4 u_Color;
    uint u_ColorInputTransfer;
    uint u_ColorOutputTransfer;
} pc;

void main()
{
    vec3 l_LinearColor = ApplySceneColorInput(v_Color.rgb, pc.u_ColorInputTransfer);
    o_Color = ApplySceneColorOutput(vec4(l_LinearColor, v_Color.a), pc.u_ColorOutputTransfer);
}