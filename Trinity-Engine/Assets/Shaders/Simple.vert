#version 450

layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 View;
    mat4 Proj;
    mat4 ViewProj;
} u_Global;

layout(location = 0) in vec3 aPosition;

layout(location = 0) out vec3 vColor;

void main()
{
    vec3 colors[3] = vec3[](
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 1.0)
    );

    vec4 viewPosition = u_Global.View * vec4(aPosition, 1.0);
    vec4 projPosition = u_Global.Proj * viewPosition;

    gl_Position = u_Global.ViewProj * vec4(aPosition, 1.0);
    vColor = colors[gl_VertexIndex] * projPosition.w;
}