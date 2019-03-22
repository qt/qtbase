#version 440

layout(location = 0) in vec3 v_color;

layout(location = 0) out vec4 c0;
layout(location = 1) out vec4 c1;
layout(location = 2) out vec4 c2;
layout(location = 3) out vec4 c3;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float opacity;
} ubuf;

void main()
{
    vec4 c = vec4(v_color * ubuf.opacity, ubuf.opacity);
    c0 = vec4(c.r, 0, 0, c.a);
    c1 = vec4(0, c.g, 0, c.a);
    c2 = vec4(0, 0, c.b, c.a);
    c3 = c;
}
