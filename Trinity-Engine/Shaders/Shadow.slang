struct VertexInput
{
    [[vk::location(0)]] float3 Position;
};

struct PushConstants
{
    float4x4 MVP;  // light view-projection * model
};

[[vk::push_constant]] PushConstants pushConstants;

[shader("vertex")]
float4 vertexMain(VertexInput input) : SV_Position
{
    return mul(pushConstants.MVP, float4(input.Position, 1.0));
}

[shader("fragment")]
void fragmentMain()
{

}