#version 440

layout(location = 0) in vec4 position;
layout(location = 0) out float v_color;

layout(std140, binding = 1) uniform buf {
    float color;
};

void main()
{
    gl_Position = position;
    v_color = color;
}
