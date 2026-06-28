struct VertexOutput
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float2 NDC;
};

struct PushConstants
{
    float4 Forward;  // xyz face forward, w = roughness
    float4 Right;
    float4 Up;
};

[[vk::push_constant]] PushConstants pc;
[[vk::binding(0, 0)]] Sampler2D u_Equirect;

static const float PI = 3.14159265359;

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.NDC = uv * 2.0 - 1.0;
    output.Position = float4(output.NDC, 0.0, 1.0);

    return output;
}

float2 SampleSphericalMap(float3 dir)
{
    float2 uv = float2(atan2(dir.z, dir.x), asin(clamp(dir.y, -1.0, 1.0)));
    uv *= float2(0.1591549, 0.3183099);
    uv += 0.5;

    return float2(uv.x, 1.0 - uv.y);
}

float3 SampleEnvironment(float3 dir)
{
    return u_Equirect.SampleLevel(SampleSphericalMap(dir), 0.0).rgb;
}

float RadicalInverseVdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint count)
{
    return float2(float(i) / float(count), RadicalInverseVdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 N = normalize(pc.Forward.xyz + input.NDC.x * pc.Right.xyz + input.NDC.y * pc.Up.xyz);
    float3 R = N;
    float3 V = N;
    float roughness = pc.Forward.w;

    const uint SAMPLE_COUNT = 256u;
    float3 prefiltered = float3(0.0, 0.0, 0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            prefiltered += SampleEnvironment(L) * NdotL;
            totalWeight += NdotL;
        }
    }

    prefiltered /= max(totalWeight, 0.001);

    return float4(prefiltered, 1.0);
}