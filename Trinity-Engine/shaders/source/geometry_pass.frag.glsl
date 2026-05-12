#version 450

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec3 v_WorldNormal;
layout(location = 2) in vec2 v_UV;
layout(location = 3) in vec3 v_WorldTangent;

layout(set = 0, binding = 0) uniform sampler2D u_AlbedoTexture;

layout(push_constant) uniform PushBlock
{
    mat4 Model;
    mat4 ViewProjection;

    vec4 BaseColor;

    vec4 MaterialData;
    vec4 EmissiveColorStrength;

    vec4 TextureFlags;
} u_Push;

layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec4 out_Normal;
layout(location = 2) out vec4 out_MetallicRoughnessAO;

void main()
{
    float l_UseAlbedoTexture = u_Push.TextureFlags.x;

    vec4 l_TextureColor = texture(u_AlbedoTexture, v_UV);
    vec4 l_BaseColor = mix(u_Push.BaseColor, u_Push.BaseColor * l_TextureColor, l_UseAlbedoTexture);

    float l_Metallic = clamp(u_Push.MaterialData.x, 0.0, 1.0);
    float l_Roughness = clamp(u_Push.MaterialData.y, 0.02, 1.0);
    float l_AmbientOcclusion = clamp(u_Push.MaterialData.z, 0.0, 1.0);

    vec3 l_Normal = normalize(v_WorldNormal);

    out_Albedo = l_BaseColor;
    out_Normal = vec4(l_Normal * 0.5 + 0.5, 1.0);
    out_MetallicRoughnessAO = vec4(l_Metallic, l_Roughness, l_AmbientOcclusion, 0.0);
}