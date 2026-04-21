#version 450

layout(set = 0, binding = 0) uniform sampler2D u_Albedo;
layout(set = 0, binding = 1) uniform sampler2D u_Normal;
layout(set = 0, binding = 2) uniform sampler2D u_MRA;
layout(set = 0, binding = 3) uniform sampler2D u_Depth;
layout(set = 0, binding = 4) uniform sampler2D u_ShadowMap;

layout(set = 0, binding = 5) uniform LightingUniforms
{
    mat4 u_InvViewProjection;
    mat4 u_LightSpaceMatrix;
    vec4 u_LightDirection;
    vec4 u_LightColor;
    vec4 u_CameraPosition;
} u_Lighting;

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 o_Color;

const float PI = 3.14159265359;

float DistributionGGX(vec3 normal, vec3 halfway, float roughness)
{
    float l_A  = roughness * roughness;
    float l_A2 = l_A * l_A;
    float l_NdotH  = max(dot(normal, halfway), 0.0);
    float l_NdotH2 = l_NdotH * l_NdotH;
    float l_Denom  = l_NdotH2 * (l_A2 - 1.0) + 1.0;

    return l_A2 / (PI * l_Denom * l_Denom);
}

float GeometrySchlickGGX(float ndotv, float roughness)
{
    float l_R = roughness + 1.0;
    float l_K = (l_R * l_R) / 8.0;

    return ndotv / (ndotv * (1.0 - l_K) + l_K);
}

float GeometrySmith(vec3 normal, vec3 view, vec3 light, float roughness)
{
    return GeometrySchlickGGX(max(dot(normal, view), 0.0), roughness) * GeometrySchlickGGX(max(dot(normal, light), 0.0), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ShadowFactor(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 l_Proj = fragPosLightSpace.xyz / fragPosLightSpace.w;
    l_Proj.xy = l_Proj.xy * 0.5 + 0.5;

    if (l_Proj.x < 0.0 || l_Proj.x > 1.0 || l_Proj.y < 0.0 || l_Proj.y > 1.0 || l_Proj.z > 1.0)
    {
        return 0.0;
    }

    float l_Bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    float l_ClosestDepth = texture(u_ShadowMap, l_Proj.xy).r;

    return l_Proj.z - l_Bias > l_ClosestDepth ? 1.0 : 0.0;
}

void main()
{
    float l_Depth = texture(u_Depth, v_TexCoord).r;
    if (l_Depth >= 1.0)
    {
        o_Color = vec4(0.0, 0.0, 0.0, 1.0);

        return;
    }

    vec3 l_Albedo = pow(texture(u_Albedo, v_TexCoord).rgb, vec3(2.2));
    vec3 l_Normal = normalize(texture(u_Normal, v_TexCoord).rgb * 2.0 - 1.0);
    vec3 l_MRASample = texture(u_MRA, v_TexCoord).rgb;
    float l_Metallic = l_MRASample.r;
    float l_Roughness = l_MRASample.g;
    float l_AO = l_MRASample.b;

    vec4 l_NDC = vec4(v_TexCoord * 2.0 - 1.0, l_Depth, 1.0);
    vec4 l_WorldPos4 = u_Lighting.u_InvViewProjection * l_NDC;
    vec3 l_WorldPos = l_WorldPos4.xyz / l_WorldPos4.w;

    vec3 l_V = normalize(u_Lighting.u_CameraPosition.xyz - l_WorldPos);
    vec3 l_L = normalize(-u_Lighting.u_LightDirection.xyz);
    vec3 l_H = normalize(l_V + l_L);
    vec3 l_F0 = mix(vec3(0.04), l_Albedo, l_Metallic);

    float l_NDF = DistributionGGX(l_Normal, l_H, l_Roughness);
    float l_G = GeometrySmith(l_Normal, l_V, l_L, l_Roughness);
    vec3 l_F = FresnelSchlick(max(dot(l_H, l_V), 0.0), l_F0);

    vec3 l_Specular  = (l_NDF * l_G * l_F) / (4.0 * max(dot(l_Normal, l_V), 0.0) * max(dot(l_Normal, l_L), 0.0) + 0.0001);
    vec3 l_kD = (vec3(1.0) - l_F) * (1.0 - l_Metallic);
    float l_NdotL = max(dot(l_Normal, l_L), 0.0);
    vec3 l_Radiance = u_Lighting.u_LightColor.rgb * u_Lighting.u_LightColor.a;

    float l_Shadow = ShadowFactor(u_Lighting.u_LightSpaceMatrix * vec4(l_WorldPos, 1.0), l_Normal, l_L);

    vec3 l_Lo = (l_kD * l_Albedo / PI + l_Specular) * l_Radiance * l_NdotL * (1.0 - l_Shadow);
    vec3 l_Ambient = vec3(0.03) * l_Albedo * l_AO;
    vec3 l_Result = l_Ambient + l_Lo;

    l_Result = l_Result / (l_Result + vec3(1.0));
    l_Result = pow(l_Result, vec3(1.0 / 2.2));

    o_Color = vec4(l_Result, 1.0);
}