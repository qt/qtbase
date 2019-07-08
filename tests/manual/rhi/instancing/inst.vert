#version 440

layout(location = 0) in vec4 position;

// Instanced attributes to variate the translation and color of the cube
layout(location = 1) in vec3 instTranslate;
layout(location = 2) in vec3 instColor;

layout(location = 0) out vec3 vColor;

out gl_PerVertex { vec4 gl_Position; };

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
} ubuf;

void main()
{
    vColor = instColor;
    mat4 t = mat4(1, 0, 0, 0,
                  0, 1, 0, 0,
                  0, 0, 1, 0,
                  instTranslate.x, instTranslate.y, instTranslate.z, 1);
    gl_Position = ubuf.mvp * t * position;
}
