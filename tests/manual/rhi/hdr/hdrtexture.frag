#version 440

layout(location = 0) in vec2 v_texcoord;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    int flip;
    int sdr;
    float tonemapInMax;
    float tonemapOutMax;
} ubuf;

layout(binding = 1) uniform sampler2D tex;

vec3 linearToSRGB(vec3 color)
{
    vec3 S1 = sqrt(color);
    vec3 S2 = sqrt(S1);
    vec3 S3 = sqrt(S2);
    return 0.585122381 * S1 + 0.783140355 * S2 - 0.368262736 * S3;
}

// https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
vec3 simpleReinhardTonemapper(vec3 c, float inMax, float outMax)
{
    c /= vec3(inMax);
    c.r = c.r / (1 + c.r);
    c.g = c.g / (1 + c.g);
    c.b = c.b / (1 + c.b);
    c *= vec3(outMax);
    return c;
}

void main()
{
    vec4 c = texture(tex, v_texcoord);
    if (ubuf.tonemapInMax != 0)
        c.rgb = simpleReinhardTonemapper(c.rgb, ubuf.tonemapInMax, ubuf.tonemapOutMax);
    else if (ubuf.sdr != 0)
        c.rgb = linearToSRGB(c.rgb);
    fragColor = vec4(c.rgb * c.a, c.a);
}
