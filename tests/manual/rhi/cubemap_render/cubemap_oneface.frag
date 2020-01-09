#version 440

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 color;
} ubuf;

void main()
{
    fragColor = vec4(ubuf.color, 1.0);
}
