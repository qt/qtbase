#version 440

layout(location = 0) out vec4 c0;
layout(location = 1) out vec4 c1;
layout(location = 2) out vec4 c2;
layout(location = 3) out vec4 c3;
layout(location = 4) out vec4 c4;
layout(location = 5) out vec4 c5;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 color0;
    vec3 color1;
    vec3 color2;
    vec3 color3;
    vec3 color4;
    vec3 color5;
} ubuf;

void main()
{
    c0 = vec4(ubuf.color0, 1.0);
    c1 = vec4(ubuf.color1, 1.0);
    c2 = vec4(ubuf.color2, 1.0);
    c3 = vec4(ubuf.color3, 1.0);
    c4 = vec4(ubuf.color4, 1.0);
    c5 = vec4(ubuf.color5, 1.0);
}
