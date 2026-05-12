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
    vec4 l_AlbedoSample = texture(u_AlbedoTexture, v_UV);
    vec3 l_Albedo = l_AlbedoSample.rgb;
    vec3 l_Normal = DecodeNormal(texture(u_NormalTexture, v_UV).rgb);
    vec3 l_MetallicRoughnessAO = texture(u_MetallicRoughnessAOTexture, v_UV).rgb;

    float l_Depth = texture(u_DepthTexture, v_UV).r;

    if (l_Depth >= 1.0)
    {
        out_Color = vec4(0.01, 0.01, 0.01, 1.0);
        return;
    }

    float l_Metallic = clamp(l_MetallicRoughnessAO.r, 0.0, 1.0);
    float l_Roughness = clamp(l_MetallicRoughnessAO.g, 0.02, 1.0);
    float l_AmbientOcclusion = clamp(l_MetallicRoughnessAO.b, 0.0, 1.0);

    vec3 l_LightDirection = normalize(-u_Push.SunDirection.xyz);
    vec3 l_LightColor = u_Push.SunColorIntensity.rgb;
    float l_LightIntensity = u_Push.SunColorIntensity.a;

    float l_NdotL = max(dot(l_Normal, l_LightDirection), 0.0);

    vec3 l_DielectricDiffuse = l_Albedo;
    vec3 l_MetalDiffuse = vec3(0.0);
    vec3 l_DiffuseColor = mix(l_DielectricDiffuse, l_MetalDiffuse, l_Metallic);

    float l_RoughnessDiffuseBoost = mix(1.0, 0.65, l_Roughness);

    vec3 l_Ambient = l_Albedo * 0.08 * l_AmbientOcclusion;
    vec3 l_Diffuse = l_DiffuseColor * l_LightColor * l_LightIntensity * l_NdotL * l_RoughnessDiffuseBoost;

    vec3 l_SpecularColor = mix(vec3(0.04), l_Albedo, l_Metallic);
    float l_SpecularStrength = (1.0 - l_Roughness) * l_NdotL;
    vec3 l_Specular = l_SpecularColor * l_LightColor * l_LightIntensity * l_SpecularStrength * 0.25;

    vec3 l_Color = l_Ambient + l_Diffuse + l_Specular;

    l_Color = l_Color / (l_Color + vec3(1.0));

    out_Color = vec4(l_Color, l_AlbedoSample.a);
}