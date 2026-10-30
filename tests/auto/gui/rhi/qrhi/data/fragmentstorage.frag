#version 440

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;

layout(std430, binding = 0) buffer counterBuf
{
    uint counter;
};

void main()
{
    atomicAdd(counter, 1u);
    fragColor = vec4(v_uv, 0.0, 1.0);
}
