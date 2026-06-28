struct VertexOutput
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float2 NDC;
};

struct PushConstants
{
    float4 Forward;  // xyz face forward
    float4 Right;    // xyz face right
    float4 Up;       // xyz face up
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

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 N = normalize(pc.Forward.xyz + input.NDC.x * pc.Right.xyz + input.NDC.y * pc.Up.xyz);

    float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float3 irradiance = float3(0.0, 0.0, 0.0);
    float sampleDelta = 0.025;
    float samples = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            float3 tangent = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sampleVec = tangent.x * right + tangent.y * up + tangent.z * N;

            irradiance += SampleEnvironment(sampleVec) * cos(theta) * sin(theta);
            samples += 1.0;
        }
    }

    irradiance = PI * irradiance * (1.0 / samples);

    return float4(irradiance, 1.0);
}