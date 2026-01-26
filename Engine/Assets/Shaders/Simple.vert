#version 450

layout(location = 0) out vec3 vColor;

layout(set = 0, binding = 1) readonly buffer TransformBuffer
{
    mat4 transforms[];
};

layout(push_constant) uniform Push
{
    uint TransformIndex;
};

vec2 positions[3] = vec2[]
(   
    vec2( 0.0, -0.5),
    vec2( 0.5,  0.5),
    vec2(-0.5,  0.5)
);

vec3 colors[3] = vec3[]
(
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main()
{
    vec4 l_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    gl_Position = transforms[TransformIndex] * l_Position;
    vColor = colors[gl_VertexIndex];
}