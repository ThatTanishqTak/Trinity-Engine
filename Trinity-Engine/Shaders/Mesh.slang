static const uint MAX_LIGHTS = 16;
static const float PI = 3.14159265359;

struct GpuLight
{
    float4 PositionType;    // xyz = world position, w = type (0 directional, 1 point, 2 spot)
    float4 DirectionRange;  // xyz = normalized direction, w = range
    float4 ColorIntensity;  // rgb = color, a = intensity
    float4 SpotAngles;      // x = cos(inner), y = cos(outer)
};

struct FrameData
{
    float4x4 ViewProjection;
    float4 CameraPosition;   // xyz = camera world position
    float4 AmbientAndCount;      // rgb = ambient color, a = active light count
    float4 IblParams;            // x = IBL enabled, y = max prefilter LOD
    float4x4 LightViewProjection;
    float4 ShadowParams;         // x = enabled, y = bias, w = 1 / shadow map size
    GpuLight Lights[MAX_LIGHTS];
};

struct PushConstants
{
    float4x4 Model;
    float4 BaseColorFactor;
    float4 PbrFactors;      // x = metallic, y = roughness, z = occlusionStrength, w = normalScale
    float4 EmissiveFactor;  // rgb = emissive color, a = emissive strength
};

[[vk::push_constant]] PushConstants pushConstants;
[[vk::binding(0, 0)]] ConstantBuffer<FrameData> u_Frame;
[[vk::binding(0, 1)]] Sampler2D u_BaseColor;
[[vk::binding(0, 2)]] Sampler2D u_Normal;
[[vk::binding(0, 3)]] Sampler2D u_MetallicRoughness;
[[vk::binding(0, 4)]] Sampler2D u_Emissive;
[[vk::binding(0, 5)]] SamplerCube u_Irradiance;
[[vk::binding(0, 6)]] SamplerCube u_Prefiltered;
[[vk::binding(0, 7)]] Sampler2D u_BrdfLut;
[[vk::binding(0, 8)]] Sampler2D u_ShadowMap;

struct VertexInput
{
    [[vk::location(0)]] float3 Position;
    [[vk::location(1)]] float3 Normal;
    [[vk::location(2)]] float3 Tangent;
    [[vk::location(3)]] float2 UV;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float3 WorldPosition;
    [[vk::location(1)]] float3 Normal;
    [[vk::location(2)]] float3 Tangent;
    [[vk::location(3)]] float2 UV;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    float4 worldPosition = mul(pushConstants.Model, float4(input.Position, 1.0));
    output.WorldPosition = worldPosition.xyz;
    output.Position = mul(u_Frame.ViewProjection, worldPosition);

    float3x3 normalMatrix = (float3x3)pushConstants.Model;
    output.Normal = normalize(mul(normalMatrix, input.Normal));
    output.Tangent = normalize(mul(normalMatrix, input.Tangent));
    output.UV = input.UV;

    return output;
}

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denominator = NdotH * NdotH * (a2 - 1.0) + 1.0;

    return a2 / max(PI * denominator * denominator, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 f0, float roughness)
{
    float3 fmax = max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), f0);

    return f0 + (fmax - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float4 sampledBase = u_BaseColor.Sample(input.UV) * pushConstants.BaseColorFactor;
    float3 albedo = sampledBase.rgb;

    float3 metallicRoughness = u_MetallicRoughness.Sample(input.UV).rgb;
    float metallic = clamp(metallicRoughness.b * pushConstants.PbrFactors.x, 0.0, 1.0);
    float roughness = clamp(metallicRoughness.g * pushConstants.PbrFactors.y, 0.04, 1.0);
    float occlusion = 1.0;

    // Tangent-space normal mapping. Re-orthonormalize the tangent against the interpolated normal.
    float3 geometricNormal = normalize(input.Normal);
    float3 tangent = normalize(input.Tangent - geometricNormal * dot(geometricNormal, input.Tangent));
    float3 bitangent = cross(geometricNormal, tangent);
    float3x3 tbn = float3x3(tangent, bitangent, geometricNormal);

    float3 sampledNormal = u_Normal.Sample(input.UV).rgb * 2.0 - 1.0;
    sampledNormal.xy *= pushConstants.PbrFactors.w;
    float3 normal = normalize(mul(sampledNormal, tbn));

    float3 viewDirection = normalize(u_Frame.CameraPosition.xyz - input.WorldPosition);
    float NdotV = max(dot(normal, viewDirection), 0.0001);

    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float3 outgoing = float3(0.0, 0.0, 0.0);

    uint lightCount = (uint)u_Frame.AmbientAndCount.a;
    lightCount = min(lightCount, MAX_LIGHTS);

    for (uint i = 0; i < lightCount; ++i)
    {
        GpuLight light = u_Frame.Lights[i];
        uint type = (uint)light.PositionType.w;
        float shadow = 1.0;

        float3 lightDirection;
        float attenuation = 1.0;

        if (type == 0)
        {
            // Directional: DirectionRange.xyz points from the light toward the scene.
            lightDirection = normalize(-light.DirectionRange.xyz);
        }
        else
        {
            float3 toLight = light.PositionType.xyz - input.WorldPosition;
            float distance = length(toLight);
            lightDirection = toLight / max(distance, 0.0001);

            float range = max(light.DirectionRange.w, 0.0001);
            float falloff = clamp(1.0 - pow(distance / range, 4.0), 0.0, 1.0);
            attenuation = (falloff * falloff) / max(distance * distance, 0.0001);

            if (type == 2)
            {
                float cosAngle = dot(normalize(light.DirectionRange.xyz), -lightDirection);
                float cosInner = light.SpotAngles.x;
                float cosOuter = light.SpotAngles.y;
                float cone = clamp((cosAngle - cosOuter) / max(cosInner - cosOuter, 0.0001), 0.0, 1.0);
                attenuation *= cone * cone;
            }
        }

        float3 radiance = light.ColorIntensity.rgb * light.ColorIntensity.a * attenuation;

        float3 halfVector = normalize(viewDirection + lightDirection);
        float NdotL = max(dot(normal, lightDirection), 0.0);
        float NdotH = max(dot(normal, halfVector), 0.0);
        float VdotH = max(dot(viewDirection, halfVector), 0.0);

        float distribution = DistributionGGX(NdotH, roughness);
        float geometry = GeometrySmith(NdotV, NdotL, roughness);
        float3 fresnel = FresnelSchlick(VdotH, f0);

        float3 specular = (distribution * geometry * fresnel) / max(4.0 * NdotV * NdotL, 0.0001);

        float3 diffuseFactor = (float3(1.0, 1.0, 1.0) - fresnel) * (1.0 - metallic);
        float3 diffuse = diffuseFactor * albedo / PI;

        if (type == 0 && u_Frame.ShadowParams.x > 0.5)
        {
            float4 lightClip = mul(u_Frame.LightViewProjection, float4(input.WorldPosition, 1.0));
            float3 lightProj = lightClip.xyz / lightClip.w;
            float2 shadowUV = lightProj.xy * 0.5 + 0.5;
            shadowUV.y = 1.0 - shadowUV.y;

            if (shadowUV.x >= 0.0 && shadowUV.x <= 1.0 && shadowUV.y >= 0.0 && shadowUV.y <= 1.0 && lightProj.z <= 1.0 && lightProj.z >= 0.0)
            {
                float bias = max(u_Frame.ShadowParams.y * (1.0 - NdotL), u_Frame.ShadowParams.y * 0.15);
                float texel = u_Frame.ShadowParams.w;

                float occlusion = 0.0;
                for (int sx = -1; sx <= 1; ++sx)
                {
                    for (int sy = -1; sy <= 1; ++sy)
                    {
                        float sampled = u_ShadowMap.SampleLevel(shadowUV + float2(sx, sy) * texel, 0.0).r;
                        occlusion += (lightProj.z - bias) > sampled ? 0.0 : 1.0;
                    }
                }

                shadow = occlusion / 9.0;
            }
        }

        outgoing += (diffuse + specular) * radiance * NdotL * shadow;
    }

    float3 ambient;
    if (u_Frame.IblParams.x > 0.5)
    {
        float3 F = FresnelSchlickRoughness(NdotV, f0, roughness);
        float3 kd = (1.0 - F) * (1.0 - metallic);

        float3 irradiance = u_Irradiance.Sample(normal).rgb;
        float3 diffuseIBL = irradiance * albedo;

        float3 reflection = reflect(-viewDirection, normal);
        float3 prefiltered = u_Prefiltered.SampleLevel(reflection, roughness * u_Frame.IblParams.y).rgb;
        float2 brdf = u_BrdfLut.Sample(float2(NdotV, roughness)).rg;
        float3 specularIBL = prefiltered * (F * brdf.x + brdf.y);

        ambient = (kd * diffuseIBL + specularIBL) * occlusion;
    }
    else
    {
        ambient = u_Frame.AmbientAndCount.rgb * albedo * occlusion;
    }

    float3 emissive = u_Emissive.Sample(input.UV).rgb * pushConstants.EmissiveFactor.rgb * pushConstants.EmissiveFactor.a;

    float3 color = ambient + outgoing + emissive;

    return float4(color, sampledBase.a);
}