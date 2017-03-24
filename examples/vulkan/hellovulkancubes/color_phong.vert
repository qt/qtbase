#version 440

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;

// Instanced attributes to variate the translation of the model and the diffuse
// color of the material.
layout(location = 2) in vec3 instTranslate;
layout(location = 3) in vec3 instDiffuseAdjust;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out vec3 vECVertNormal;
layout(location = 1) out vec3 vECVertPos;
layout(location = 2) flat out vec3 vDiffuseAdjust;

layout(std140, binding = 0) uniform buf {
    mat4 vp;
    mat4 model;
    mat3 modelNormal;
} ubuf;

void main()
{
    vECVertNormal = normalize(ubuf.modelNormal * normal);
    mat4 t = mat4(1, 0, 0, 0,
                  0, 1, 0, 0,
                  0, 0, 1, 0,
                  instTranslate.x, instTranslate.y, instTranslate.z, 1);
    vECVertPos = vec3(t * ubuf.model * position);
    vDiffuseAdjust = instDiffuseAdjust;
    gl_Position = ubuf.vp * t * ubuf.model * position;
}
