#version 450

layout(location = 0) in vec2 v_UV;

layout(set = 0, binding = 0) uniform sampler2D u_AlbedoTexture;
layout(set = 0, binding = 1) uniform sampler2D u_NormalTexture;
layout(set = 0, binding = 2) uniform sampler2D u_MetallicRoughnessAOTexture;
layout(set = 0, binding = 3) uniform sampler2D u_DepthTexture;
layout(set = 0, binding = 4) uniform sampler2D u_ShadowMap;

layout(push_constant) uniform PushBlock
{
    vec4 SunDirection;
    vec4 SunColorIntensity;
} u_Push;

layout(location = 0) out vec4 out_Color;

vec3 DecodeNormal(vec3 encodedNormal)
{
    return normalize(encodedNormal * 2.0 - 1.0);
}

void main()
{
    vec3 l_Albedo = texture(u_AlbedoTexture, v_UV).rgb;
    vec3 l_Normal = DecodeNormal(texture(u_NormalTexture, v_UV).rgb);
    vec3 l_MetallicRoughnessAO = texture(u_MetallicRoughnessAOTexture, v_UV).rgb;

    float l_Depth = texture(u_DepthTexture, v_UV).r;

    if (l_Depth >= 1.0)
    {
        out_Color = vec4(0.01, 0.01, 0.01, 1.0);
        return;
    }

    float l_AmbientOcclusion = l_MetallicRoughnessAO.b;

    vec3 l_LightDirection = normalize(-u_Push.SunDirection.xyz);
    vec3 l_LightColor = u_Push.SunColorIntensity.rgb;
    float l_LightIntensity = u_Push.SunColorIntensity.a;

    float l_NdotL = max(dot(l_Normal, l_LightDirection), 0.0);

    vec3 l_Ambient = l_Albedo * 0.08 * l_AmbientOcclusion;
    vec3 l_Diffuse = l_Albedo * l_LightColor * l_LightIntensity * l_NdotL;

    vec3 l_Color = l_Ambient + l_Diffuse;

    l_Color = l_Color / (l_Color + vec3(1.0));

    out_Color = vec4(l_Color, 1.0);
}