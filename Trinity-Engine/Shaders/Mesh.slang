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
    [[vk::location(0)]] float3 Normal;
    [[vk::location(1)]] float2 UV;
    [[vk::location(2)]] float3 Tangent;
};

struct PushConstants
{
    float4x4 MVP;
};

[[vk::push_constant]] PushConstants pushConstants;
[[vk::binding(0, 0)]] Sampler2D u_BaseColor;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.Position = mul(pushConstants.MVP, float4(input.Position, 1.0));
    output.Normal = input.Normal;
    output.UV = input.UV;
    output.Tangent = input.Tangent;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 baseColor = u_BaseColor.Sample(input.UV).rgb;

    return float4(baseColor, 1.0);
}