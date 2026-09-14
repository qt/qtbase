#version 440

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    vec4 tint;
} ubuf;

layout(push_constant, std140) uniform PC {
    vec4 offsetScale;
    vec4 color;
    mat3 colorMat;
    float weights[2];
} pc;

void main()
{
    // colorMat is the identity and the weights add up to 1, so this is the
    // color itself - but only if the block was read back correctly.
    float w = pc.weights[0] + pc.weights[1];
    vec3 c = pc.colorMat * pc.color.rgb;
    fragColor = vec4(c * w, pc.color.a) * ubuf.tint;
}
