struct VertexOutput
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float2 NDC;
};

struct PushConstants
{
    float4x4 InverseViewProjection;
    float4 CameraPositionIntensity;  // xyz = camera world position, w = intensity
};

[[vk::push_constant]] PushConstants pushConstants;
[[vk::binding(0, 0)]] Sampler2D u_Environment;

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.NDC = uv * 2.0 - 1.0;
    output.Position = float4(output.NDC, 1.0, 1.0);  // far plane (zero-to-one depth)

    return output;
}

float2 SampleSphericalMap(float3 dir)
{
    float2 uv = float2(atan2(dir.z, dir.x), asin(clamp(dir.y, -1.0, 1.0)));
    uv *= float2(0.1591549, 0.3183099);  // (1/2pi, 1/pi)
    uv += 0.5;

    return uv;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float4 world = mul(pushConstants.InverseViewProjection, float4(input.NDC, 1.0, 1.0));
    float3 worldPosition = world.xyz / world.w;
    float3 direction = normalize(worldPosition - pushConstants.CameraPositionIntensity.xyz);

    float2 uv = SampleSphericalMap(direction);
    float3 color = u_Environment.Sample(float2(uv.x, 1.0 - uv.y)).rgb * pushConstants.CameraPositionIntensity.w;

    return float4(color, 1.0);
}