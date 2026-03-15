#ifndef TRINITY_PBR_GLSL
#define TRINITY_PBR_GLSL

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float l_A  = roughness * roughness;
    float l_A2 = l_A * l_A;
    float l_NdotH  = max(dot(N, H), 0.0);
    float l_NdotH2 = l_NdotH * l_NdotH;

    float l_Denom = (l_NdotH2 * (l_A2 - 1.0) + 1.0);
    l_Denom = PI * l_Denom * l_Denom;

    return l_A2 / max(l_Denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float l_R = roughness + 1.0;
    float l_K = (l_R * l_R) / 8.0;
    return NdotV / (NdotV * (1.0 - l_K) + l_K);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float l_NdotV = max(dot(N, V), 0.0);
    float l_NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(l_NdotV, roughness) * GeometrySchlickGGX(l_NdotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CookTorranceBRDF(vec3 albedo, float metalness, float roughness,
    vec3 N, vec3 V, vec3 L, vec3 lightColor, float attenuation)
{
    vec3 l_F0 = mix(vec3(0.04), albedo, metalness);
    vec3 l_H  = normalize(V + L);

    float l_NdotL = max(dot(N, L), 0.0);

    float l_D = DistributionGGX(N, l_H, roughness);
    float l_G = GeometrySmith(N, V, L, roughness);
    vec3  l_F = FresnelSchlick(max(dot(l_H, V), 0.0), l_F0);

    vec3 l_Numerator   = l_D * l_G * l_F;
    float l_Denominator = 4.0 * max(dot(N, V), 0.0) * l_NdotL + 0.0001;
    vec3 l_Specular    = l_Numerator / l_Denominator;

    vec3 l_Ks = l_F;
    vec3 l_Kd = (vec3(1.0) - l_Ks) * (1.0 - metalness);

    return (l_Kd * albedo / PI + l_Specular) * lightColor * attenuation * l_NdotL;
}

float PCF3x3(sampler2DShadow shadowMap, vec4 shadowCoord, float bias)
{
    vec3 l_ProjCoord = shadowCoord.xyz / shadowCoord.w;
    l_ProjCoord.xy   = l_ProjCoord.xy * 0.5 + 0.5;
    l_ProjCoord.z   -= bias;

    if (l_ProjCoord.z > 1.0 || l_ProjCoord.x < 0.0 || l_ProjCoord.x > 1.0 ||
        l_ProjCoord.y < 0.0 || l_ProjCoord.y > 1.0)
    {
        return 1.0;
    }

    float l_Shadow = 0.0;
    vec2 l_TexelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    for (int it_X = -1; it_X <= 1; ++it_X)
    {
        for (int it_Y = -1; it_Y <= 1; ++it_Y)
        {
            vec2 l_Offset = vec2(it_X, it_Y) * l_TexelSize;
            l_Shadow += texture(shadowMap, vec3(l_ProjCoord.xy + l_Offset, l_ProjCoord.z));
        }
    }

    return l_Shadow / 9.0;
}

#endif
