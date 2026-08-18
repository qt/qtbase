#version 440

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;

layout(std140, binding = 0) uniform Ubuf {
    mat4 mvp;
} ubuf;

layout(location = 0) out vec4 vColor;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = ubuf.mvp * vec4(inPos, 1.0);
    vColor = inColor;
}
