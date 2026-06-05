struct VertexOutput
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float2 UV;
};

struct PushConstants
{
    float Exposure;
};

[[vk::push_constant]] PushConstants pushConstants;
[[vk::binding(0, 0)]] Sampler2D u_Scene;

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.UV = float2((vertexID << 1) & 2, vertexID & 2);
    output.Position = float4(output.UV * 2.0 - 1.0, 0.0, 1.0);

    return output;
}

float3 ACESFilm(float3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 LinearToSRGB(float3 color)
{
    float3 lo = color * 12.92;
    float3 hi = 1.055 * pow(max(color, 0.0), 1.0 / 2.4) - 0.055;
    float3 mask = step(0.0031308, color);

    return lerp(lo, hi, mask);
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 hdr = u_Scene.Sample(float2(input.UV.x, 1.0 - input.UV.y)).rgb * pushConstants.Exposure;
    float3 mapped = ACESFilm(hdr);
    float3 encoded = LinearToSRGB(mapped);

    return float4(encoded, 1.0);
}