#version 450

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec3 v_WorldNormal;
layout(location = 2) in vec2 v_UV;

layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec4 out_Normal;
layout(location = 2) out vec4 out_MetallicRoughnessAO;

vec3 SampleAlbedo(vec2 uv)
{
    return vec3(uv.x, uv.y, 1.0 - uv.x);
}

vec3 SampleNormal(vec2 uv, vec3 worldNormal)
{
    return normalize(worldNormal);
}

vec3 SampleMetallicRoughnessAO(vec2 uv)
{
    float l_Metallic = 0.0;
    float l_Roughness = 0.5;
    float l_AmbientOcclusion = 1.0;
    return vec3(l_Metallic, l_Roughness, l_AmbientOcclusion);
}

void main()
{
    vec3 l_Albedo = SampleAlbedo(v_UV);
    vec3 l_Normal = SampleNormal(v_UV, v_WorldNormal);
    vec3 l_MetallicRoughnessAO = SampleMetallicRoughnessAO(v_UV);

    out_Albedo = vec4(l_Albedo, 1.0);
    out_Normal = vec4(l_Normal * 0.5 + 0.5, 1.0);
    out_MetallicRoughnessAO = vec4(l_MetallicRoughnessAO, 0.0);
}
