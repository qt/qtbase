#version 440

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    float opacity;
} ubuf;

void main()
{
    fragColor = vec4(0.0, ubuf.opacity, 0.0, ubuf.opacity);
}
