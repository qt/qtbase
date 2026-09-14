#version 440

layout(location = 0) in vec4 position;

// std140 because a push constant block defaults to std430, where an array of
// scalars or of vec2 is tightly packed, and that cannot be expressed in an
// HLSL cbuffer. The matrix and the array are here to exercise the unpacking
// on the backends that have to take the block apart themselves.
layout(push_constant, std140) uniform PC {
    vec4 offsetScale;
    vec4 color;
    mat3 colorMat;
    float weights[2];
} pc;

void main()
{
    gl_Position = vec4(position.xy * pc.offsetScale.zw + pc.offsetScale.xy, 0.0, 1.0);
}
