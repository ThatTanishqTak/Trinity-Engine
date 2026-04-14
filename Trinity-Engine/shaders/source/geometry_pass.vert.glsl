#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

layout(location = 0) out vec3 v_FragPos;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;

layout(push_constant) uniform PushConstants
{
    mat4 u_Model;
    mat4 u_ViewProjection;
} u_Push;

void main()
{
    vec4 l_WorldPos = u_Push.u_Model * vec4(a_Position, 1.0);
    v_FragPos       = l_WorldPos.xyz;
    v_Normal        = mat3(transpose(inverse(u_Push.u_Model))) * a_Normal;
    v_TexCoord      = a_TexCoord;
    gl_Position     = u_Push.u_ViewProjection * l_WorldPos;
}
