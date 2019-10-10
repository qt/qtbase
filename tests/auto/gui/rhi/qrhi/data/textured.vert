#version 440

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;

layout(location = 0) out vec2 uv;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    uv = texcoord;
    gl_Position = ubuf.matrix * position;
}
