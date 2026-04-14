#version 450

// Gap — deferred lighting resolve pass (Phase lighting).
layout(location = 0) out vec4 o_Color;

void main()
{
    o_Color = vec4(0.0);
}
