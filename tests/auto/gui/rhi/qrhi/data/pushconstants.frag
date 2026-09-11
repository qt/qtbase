#version 440

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    vec4 tint;
} ubuf;

layout(push_constant) uniform PC {
    vec4 offsetScale;
    vec4 color;
} pc;

void main()
{
    fragColor = pc.color * ubuf.tint;
}
