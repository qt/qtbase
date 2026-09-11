#version 440

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PC {
    vec4 translation;
    vec4 color;
} pc;

void main()
{
    fragColor = pc.color;
}
