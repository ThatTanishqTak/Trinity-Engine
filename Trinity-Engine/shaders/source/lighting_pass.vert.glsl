#version 450

layout(location = 0) out vec2 v_TexCoord;

void main()
{
    vec2 l_UV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    v_TexCoord = l_UV;
    gl_Position = vec4(l_UV * 2.0 - 1.0, 0.0, 1.0);
}