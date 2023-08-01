#version 440

layout(location = 0) in vec2 v_texcoord;
layout(location = 1) in vec4 v_color;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float opacity;
    // Windows HDR: set to SDR_white_level_in_nits / 80
    // macOS/iOS EDR: set to 1.0
    // No HDR: set to 0.0, will do linear to sRGB at the end then.
    float hdrWhiteLevelMult;
};

layout(binding = 1) uniform sampler2D tex;

vec3 linearToSRGB(vec3 color)
{
    vec3 S1 = sqrt(color);
    vec3 S2 = sqrt(S1);
    vec3 S3 = sqrt(S2);
    return 0.585122381 * S1 + 0.783140355 * S2 - 0.368262736 * S3;
}

void main()
{
    vec4 c = v_color * texture(tex, v_texcoord);
    c.a *= opacity;
    if (hdrWhiteLevelMult > 0.0)
        c.rgb *= hdrWhiteLevelMult;
    else
        c.rgb = linearToSRGB(c.rgb);
    c.rgb *= c.a; // premultiplied alpha
    fragColor = c;
}
