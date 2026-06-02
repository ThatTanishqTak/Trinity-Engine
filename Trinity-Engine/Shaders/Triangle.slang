struct VertexInput
{
    [[vk::location(0)]] float3 Position;
    [[vk::location(1)]] float3 Color;
    [[vk::location(2)]] float2 UV;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float3 Color;
    [[vk::location(1)]] float2 UV;
};

struct PushConstants
{
    float4x4 MVP;
};

[[vk::push_constant]] PushConstants pc;

[[vk::binding(0, 0)]] Sampler2D u_Texture;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.Position = mul(pc.MVP, float4(input.Position, 1.0));
    output.Color = input.Color;
    output.UV = input.UV;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 l_Texel = u_Texture.Sample(input.UV).rgb;

    return float4(l_Texel * input.Color, 1.0);
}