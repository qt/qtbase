#version 440

layout(location = 0) in vec4 vLCVertPos;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2DShadow shadowMap;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    mat4 lightMvp;
    mat4 shadowBias;
    int useShadow;
} ubuf;

void main()
{
    vec4 adjustedLcVertPos = vLCVertPos;
    adjustedLcVertPos.z -= 0.0001; // bias to avoid acne

    // no textureProj, that seems to end up not doing the perspective divide for z (?)
    vec3 v = adjustedLcVertPos.xyz / adjustedLcVertPos.w;
    float sc = texture(shadowMap, v); // sampler is comparison enabled so compares to z

    float shadowFactor = 0.2;
    if (sc > 0 || ubuf.useShadow == 0)
        shadowFactor = 1.0;

    fragColor = vec4(0.5, 0.3 + ubuf.useShadow * 0.2, 0.7, 1.0) * shadowFactor;
}
