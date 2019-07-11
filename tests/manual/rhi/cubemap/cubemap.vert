#version 440

layout(location = 0) in vec4 position;
layout(location = 0) out vec3 v_coord;
layout(std140, binding = 0) uniform buf {
    mat4 mvp;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    v_coord = position.xyz;
    gl_Position = ubuf.mvp * position;
}
