#version 440

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf
{
    vec4 color;
};

void main()
{
    fragColor = color;
}
