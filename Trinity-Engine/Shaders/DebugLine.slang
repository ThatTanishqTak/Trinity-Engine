struct VertexInput
{
    [[vk::location(0)]] float3 Position;
    [[vk::location(1)]] float4 Color;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float4 Color;
};

struct PushConstants
{
    float4x4 ViewProjection;
};

[[vk::push_constant]] PushConstants pushConstants;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.Position = mul(pushConstants.ViewProjection, float4(input.Position, 1.0));
    output.Color = input.Color;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return input.Color;
}