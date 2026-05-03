#version 440

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 vColor;

layout(std140, binding = 0) uniform Ubuf {
    mat4 mvp;
} ubuf;

out gl_PerVertex { vec4 gl_Position; float gl_PointSize; };

void main()
{
    gl_PointSize = 3.0;
    gl_Position = ubuf.mvp * position;
    vColor = color;
}
