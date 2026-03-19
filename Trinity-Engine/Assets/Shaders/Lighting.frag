#version 450

#include "PBR.glsl"
#include "ColorOutput.glsl"

layout(set = 0, binding = 0) uniform sampler2D u_Albedo;
layout(set = 0, binding = 1) uniform sampler2D u_Normal;
layout(set = 0, binding = 2) uniform sampler2D u_Material;
layout(set = 0, binding = 3) uniform sampler2D u_Depth;
layout(set = 0, binding = 4) uniform sampler2DShadow u_ShadowMap;

struct LightData
{
    vec4 PositionAndType;
    vec4 DirectionAndIntensity;
    vec4 ColorAndRange;
    vec4 SpotAngles;
};

layout(set = 1, binding = 0) uniform LightingUniforms
{
    LightData u_Lights[32];
    mat4 u_DirectionalLightSpaceMatrix;
    uint u_LightCount;
    uint u_ShadowsEnabled;
    uint _pad[2];
} u_LightData;

layout(push_constant) uniform PushConstants
{
    mat4 u_InvViewProjection;
    vec4 u_CameraPosition;
    uint u_ColorOutputTransfer;
    float u_CameraNear;
    float u_CameraFar;
    uint _pad;
} pc;

layout(location = 0) in vec2 v_UV;
layout(location = 0) out vec4 o_Color;

vec3 ReconstructWorldPosition(vec2 uv, float depth)
{
    vec4 l_ClipPos  = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 l_WorldPos = pc.u_InvViewProjection * l_ClipPos;
    return l_WorldPos.xyz / l_WorldPos.w;
}

void main()
{
    float l_Depth = texture(u_Depth, v_UV).r;

    if (l_Depth >= 1.0)
    {
        o_Color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4  l_AlbedoSample = texture(u_Albedo, v_UV);
    vec4  l_NormalSample = texture(u_Normal, v_UV);
    vec4  l_MaterialSample = texture(u_Material, v_UV);

    vec3  l_Albedo = l_AlbedoSample.rgb;
    vec3  l_N = normalize(l_NormalSample.rgb * 2.0 - 1.0);
    float l_Roughness = l_MaterialSample.r;
    float l_Metalness = l_MaterialSample.g;

    vec3 l_WorldPos = ReconstructWorldPosition(v_UV, l_Depth);
    vec3 l_V = normalize(pc.u_CameraPosition.xyz - l_WorldPos);

    vec3 l_Lo = vec3(0.0);

    for (uint i = 0u; i < u_LightData.u_LightCount; ++i)
    {
        LightData l_Light = u_LightData.u_Lights[i];

        uint  l_Type = uint(l_Light.PositionAndType.w);
        float l_Intensity = l_Light.DirectionAndIntensity.w;
        vec3  l_Color = l_Light.ColorAndRange.rgb;
        float l_Range = l_Light.ColorAndRange.w;

        vec3  l_L;
        float l_Attenuation;

        if (l_Type == 0u)
        {
            l_L = normalize(-l_Light.DirectionAndIntensity.xyz);
            l_Attenuation = l_Intensity;

            if (u_LightData.u_ShadowsEnabled != 0u && i == 0u)
            {
                vec4 l_ShadowCoord = u_LightData.u_DirectionalLightSpaceMatrix * vec4(l_WorldPos, 1.0);
                float l_ShadowFactor = PCF3x3(u_ShadowMap, l_ShadowCoord, 0.005);
                l_Attenuation *= l_ShadowFactor;
            }
        }
        else if (l_Type == 1u)
        {
            vec3  l_LightPos = l_Light.PositionAndType.xyz;
            vec3  l_ToLight = l_LightPos - l_WorldPos;
            float l_Dist = length(l_ToLight);
            l_L = l_ToLight / max(l_Dist, 0.0001);
            float l_FallOff = clamp(1.0 - pow(l_Dist / max(l_Range, 0.0001), 4.0), 0.0, 1.0);
            l_Attenuation = l_Intensity * l_FallOff * l_FallOff / (l_Dist * l_Dist + 1.0);
        }
        else
        {
            vec3  l_LightPos  = l_Light.PositionAndType.xyz;
            vec3  l_LightDir  = normalize(l_Light.DirectionAndIntensity.xyz);
            vec3  l_ToLight   = l_LightPos - l_WorldPos;
            float l_Dist      = length(l_ToLight);
            l_L               = l_ToLight / max(l_Dist, 0.0001);

            float l_CosAngle     = dot(-l_L, l_LightDir);
            float l_InnerCos     = l_Light.SpotAngles.x;
            float l_OuterCos     = l_Light.SpotAngles.y;
            float l_SpotFactor   = clamp((l_CosAngle - l_OuterCos) / max(l_InnerCos - l_OuterCos, 0.0001), 0.0, 1.0);
            float l_FallOff      = clamp(1.0 - pow(l_Dist / max(l_Range, 0.0001), 4.0), 0.0, 1.0);
            l_Attenuation        = l_Intensity * l_SpotFactor * l_FallOff * l_FallOff / (l_Dist * l_Dist + 1.0);
        }

        l_Lo += CookTorranceBRDF(l_Albedo, l_Metalness, l_Roughness, l_N, l_V, l_L, l_Color, l_Attenuation);
    }

    const vec3 l_Ambient = vec3(0.15) * l_Albedo;
    vec3 l_FinalColor = l_Ambient + l_Lo;

    o_Color = ApplySceneColorOutput(vec4(l_FinalColor, 1.0), pc.u_ColorOutputTransfer);
}