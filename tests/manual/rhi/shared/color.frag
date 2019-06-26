#version 440

layout(location = 0) in vec3 v_color;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float opacity;
} ubuf;

void main()
{
    fragColor = vec4(v_color * ubuf.opacity, ubuf.opacity);
}
