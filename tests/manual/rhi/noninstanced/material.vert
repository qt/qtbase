#version 440

layout(location = 0) in vec4 position;

layout(location = 0) out vec3 vColor;

out gl_PerVertex { vec4 gl_Position; };

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 translation;
    vec3 color;
} ubuf;

void main()
{
    vColor = ubuf.color;
    mat4 t = mat4(1, 0, 0, 0,
                  0, 1, 0, 0,
                  0, 0, 1, 0,
                  ubuf.translation.x, ubuf.translation.y, ubuf.translation.z, 1);
    gl_Position = ubuf.mvp * t * position;
}
