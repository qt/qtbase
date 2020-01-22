#version 440

layout(location = 0) in vec4 position;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 color0;
    vec3 color1;
    vec3 color2;
    vec3 color3;
    vec3 color4;
    vec3 color5;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = ubuf.mvp * position;
}
