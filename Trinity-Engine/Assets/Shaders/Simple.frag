#version 450

#include "ColorOutput.glsl"

layout(set = 0, binding = 0) uniform sampler2D u_Texture;

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;

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
    vec4 l_TexSample = texture(u_Texture, v_UV);
    vec4 l_FinalColor = l_TexSample * v_Color;
    vec3 l_LinearColor = ApplySceneColorInput(l_FinalColor.rgb, pc.u_ColorInputTransfer);
    o_Color = ApplySceneColorOutput(vec4(l_LinearColor, l_FinalColor.a), pc.u_ColorOutputTransfer);
}
