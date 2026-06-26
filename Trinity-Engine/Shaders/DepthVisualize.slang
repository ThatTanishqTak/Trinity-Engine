struct VertexOutput
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float2 UV;
};

struct PushConstants
{
    float Near;
    float Far;
};

[[vk::push_constant]] PushConstants pushConstants;
[[vk::binding(0, 0)]] Sampler2D u_Depth;

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.UV = float2((vertexID << 1) & 2, vertexID & 2);
    output.Position = float4(output.UV * 2.0 - 1.0, 0.0, 1.0);

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float depth = u_Depth.Sample(float2(input.UV.x, 1.0 - input.UV.y)).r;

    // Linearize a zero-to-one (Vulkan/GLM_FORCE_DEPTH_ZERO_TO_ONE) perspective depth value into view-space distance, then normalize to [0,1] Near = dark, far = white
    float near = pushConstants.Near;
    float far = pushConstants.Far;
    float linearDepth = (near * far) / max(far - depth * (far - near), 0.0001);
    float normalized = saturate(linearDepth / far);

    return float4(normalized, normalized, normalized, 1.0);
}