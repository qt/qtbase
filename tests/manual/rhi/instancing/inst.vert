#version 440

layout(location = 0) in vec4 position;

// Instanced attributes to variate the transform and color of the cube
layout(location = 1) in mat4 instMat;
layout(location = 5) in vec3 instColor;

layout(location = 0) out vec3 vColor;

out gl_PerVertex { vec4 gl_Position; };

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
} ubuf;

void main()
{
    vColor = instColor;
    gl_Position = ubuf.mvp * instMat * position;
}
