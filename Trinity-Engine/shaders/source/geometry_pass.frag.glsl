#version 450

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec3 v_WorldNormal;
layout(location = 2) in vec2 v_UV;
layout(location = 3) in vec3 v_WorldTangent;

layout(push_constant) uniform PushBlock
{
    mat4 Model;
    mat4 ViewProjection;
    vec4 BaseColor;
} u_Push;

layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec4 out_Normal;
layout(location = 2) out vec4 out_MetallicRoughnessAO;

vec3 SampleAlbedo()
{
    return u_Push.BaseColor.rgb;
}

vec3 SampleNormal(vec3 worldNormal)
{
    return normalize(worldNormal);
}

vec3 SampleMetallicRoughnessAO()
{
    float l_Metallic = 0.0;
    float l_Roughness = 0.5;
    float l_AmbientOcclusion = 1.0;
    return vec3(l_Metallic, l_Roughness, l_AmbientOcclusion);
}

void main()
{
    vec3 l_Albedo = SampleAlbedo();
    vec3 l_Normal = SampleNormal(v_WorldNormal);
    vec3 l_MetallicRoughnessAO = SampleMetallicRoughnessAO();

    out_Albedo = vec4(l_Albedo, u_Push.BaseColor.a);
    out_Normal = vec4(l_Normal * 0.5 + 0.5, 1.0);
    out_MetallicRoughnessAO = vec4(l_MetallicRoughnessAO, 0.0);
}