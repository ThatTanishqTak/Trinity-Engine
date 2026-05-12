#version 450

layout(location = 0) out vec2 v_UV;

void main()
{
    vec2 l_Positions[3] = vec2[]
    (
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );

    vec2 l_Position = l_Positions[gl_VertexIndex];

    v_UV = l_Position * 0.5 + 0.5;

    gl_Position = vec4(l_Position, 0.0, 1.0);
}