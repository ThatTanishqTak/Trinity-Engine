#version 450

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec3 v_WorldNormal;
layout(location = 2) in vec2 v_UV;
layout(location = 3) in vec3 v_WorldTangent;

layout(set = 0, binding = 0) uniform sampler2D u_AlbedoTexture;
layout(set = 0, binding = 1) uniform sampler2D u_NormalTexture;

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

vec3 DecodeNormalMap(vec3 normalSample)
{
    return normalize(normalSample * 2.0 - 1.0);
}

vec3 BuildMappedNormal(vec3 vertexNormal, vec3 vertexTangent, vec2 uv)
{
    vec3 l_Normal = normalize(vertexNormal);
    vec3 l_Tangent = vertexTangent;

    if (length(l_Tangent) < 0.0001)
    {
        vec3 l_Dp1 = dFdx(v_WorldPosition);
        vec3 l_Dp2 = dFdy(v_WorldPosition);
        vec2 l_Duv1 = dFdx(uv);
        vec2 l_Duv2 = dFdy(uv);

        float l_Determinant = l_Duv1.x * l_Duv2.y - l_Duv1.y * l_Duv2.x;

        if (abs(l_Determinant) > 0.000001)
        {
            float l_InverseDeterminant = 1.0 / l_Determinant;
            l_Tangent = normalize((l_Dp1 * l_Duv2.y - l_Dp2 * l_Duv1.y) * l_InverseDeterminant);
        }
        else
        {
            l_Tangent = normalize(cross(vec3(0.0, 1.0, 0.0), l_Normal));

            if (length(l_Tangent) < 0.0001)
            {
                l_Tangent = normalize(cross(vec3(1.0, 0.0, 0.0), l_Normal));
            }
        }
    }
    else
    {
        l_Tangent = normalize(l_Tangent);
    }

    l_Tangent = normalize(l_Tangent - l_Normal * dot(l_Normal, l_Tangent));

    vec3 l_Bitangent = normalize(cross(l_Normal, l_Tangent));
    mat3 l_TBN = mat3(l_Tangent, l_Bitangent, l_Normal);

    vec3 l_TangentSpaceNormal = DecodeNormalMap(texture(u_NormalTexture, uv).rgb);

    return normalize(l_TBN * l_TangentSpaceNormal);
}

void main()
{
    float l_UseAlbedoTexture = u_Push.TextureFlags.x;
    float l_UseNormalTexture = u_Push.TextureFlags.y;

    vec4 l_TextureColor = texture(u_AlbedoTexture, v_UV);
    vec4 l_BaseColor = u_Push.BaseColor;

    if (l_UseAlbedoTexture > 0.5)
    {
        l_BaseColor *= l_TextureColor;
    }

    float l_Metallic = clamp(u_Push.MaterialData.x, 0.0, 1.0);
    float l_Roughness = clamp(u_Push.MaterialData.y, 0.02, 1.0);
    float l_AmbientOcclusion = clamp(u_Push.MaterialData.z, 0.0, 1.0);

    vec3 l_Normal = normalize(v_WorldNormal);

    if (l_UseNormalTexture > 0.5)
    {
        l_Normal = BuildMappedNormal(v_WorldNormal, v_WorldTangent, v_UV);
    }

    out_Albedo = l_BaseColor;
    out_Normal = vec4(l_Normal * 0.5 + 0.5, 1.0);
    out_MetallicRoughnessAO = vec4(l_Metallic, l_Roughness, l_AmbientOcclusion, 0.0);
}