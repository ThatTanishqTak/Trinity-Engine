#version 450

layout(location = 0) in vec2 v_UV;

layout(set = 0, binding = 0) uniform sampler2D u_AlbedoTexture;
layout(set = 0, binding = 1) uniform sampler2D u_NormalTexture;
layout(set = 0, binding = 2) uniform sampler2D u_MetallicRoughnessAOTexture;
layout(set = 0, binding = 3) uniform sampler2D u_DepthTexture;

layout(push_constant) uniform PushBlock
{
    mat4 InverseViewProjection;

    vec4 CameraPosition;
    vec4 ScreenSize;

    vec4 SunDirection;
    vec4 SunColorIntensity;
} u_Push;

layout(location = 0) out vec4 out_Color;

const float PI = 3.14159265359;

vec3 DecodeNormal(vec3 encodedNormal)
{
    return normalize(encodedNormal * 2.0 - 1.0);
}

vec3 ReconstructWorldPosition(vec2 uv, float depth)
{
    vec2 l_NDC = uv * 2.0 - 1.0;
    vec4 l_ClipPosition = vec4(l_NDC, depth, 1.0);
    vec4 l_WorldPosition = u_Push.InverseViewProjection * l_ClipPosition;

    return l_WorldPosition.xyz / l_WorldPosition.w;
}

float DistributionGGX(vec3 normal, vec3 halfway, float roughness)
{
    float l_A = roughness * roughness;
    float l_A2 = l_A * l_A;
    float l_NdotH = max(dot(normal, halfway), 0.0);
    float l_NdotH2 = l_NdotH * l_NdotH;

    float l_Numerator = l_A2;
    float l_Denominator = l_NdotH2 * (l_A2 - 1.0) + 1.0;
    l_Denominator = PI * l_Denominator * l_Denominator;

    return l_Numerator / max(l_Denominator, 0.0001);
}

float GeometrySchlickGGX(float ndotv, float roughness)
{
    float l_R = roughness + 1.0;
    float l_K = (l_R * l_R) / 8.0;

    float l_Numerator = ndotv;
    float l_Denominator = ndotv * (1.0 - l_K) + l_K;

    return l_Numerator / max(l_Denominator, 0.0001);
}

float GeometrySmith(vec3 normal, vec3 viewDirection, vec3 lightDirection, float roughness)
{
    float l_NdotV = max(dot(normal, viewDirection), 0.0);
    float l_NdotL = max(dot(normal, lightDirection), 0.0);

    float l_GGXView = GeometrySchlickGGX(l_NdotV, roughness);
    float l_GGXLight = GeometrySchlickGGX(l_NdotL, roughness);

    return l_GGXView * l_GGXLight;
}

vec3 FresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 TonemapReinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

void main()
{
    vec4 l_AlbedoSample = texture(u_AlbedoTexture, v_UV);
    vec3 l_Albedo = max(l_AlbedoSample.rgb, vec3(0.0));
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

    vec3 l_WorldPosition = ReconstructWorldPosition(v_UV, l_Depth);
    vec3 l_ViewDirection = normalize(u_Push.CameraPosition.xyz - l_WorldPosition);

    vec3 l_LightDirection = normalize(-u_Push.SunDirection.xyz);
    vec3 l_LightColor = u_Push.SunColorIntensity.rgb * u_Push.SunColorIntensity.a;

    vec3 l_HalfwayDirection = normalize(l_ViewDirection + l_LightDirection);

    float l_NdotL = max(dot(l_Normal, l_LightDirection), 0.0);
    float l_NdotV = max(dot(l_Normal, l_ViewDirection), 0.0);
    float l_HdotV = max(dot(l_HalfwayDirection, l_ViewDirection), 0.0);

    vec3 l_F0 = mix(vec3(0.04), l_Albedo, l_Metallic);

    float l_NDF = DistributionGGX(l_Normal, l_HalfwayDirection, l_Roughness);
    float l_Geometry = GeometrySmith(l_Normal, l_ViewDirection, l_LightDirection, l_Roughness);
    vec3 l_Fresnel = FresnelSchlick(l_HdotV, l_F0);

    vec3 l_SpecularNumerator = l_NDF * l_Geometry * l_Fresnel;
    float l_SpecularDenominator = max(4.0 * l_NdotV * l_NdotL, 0.0001);
    vec3 l_Specular = l_SpecularNumerator / l_SpecularDenominator;

    vec3 l_KS = l_Fresnel;
    vec3 l_KD = (vec3(1.0) - l_KS) * (1.0 - l_Metallic);

    vec3 l_Diffuse = l_KD * l_Albedo / PI;

    vec3 l_DirectLighting = (l_Diffuse + l_Specular) * l_LightColor * l_NdotL;

    vec3 l_Ambient = l_Albedo * 0.03 * l_AmbientOcclusion;
    vec3 l_Color = l_Ambient + l_DirectLighting;

    l_Color = TonemapReinhard(l_Color);

    out_Color = vec4(l_Color, l_AlbedoSample.a);
}