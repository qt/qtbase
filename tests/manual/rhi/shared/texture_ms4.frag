#version 440

layout(location = 0) in vec2 v_texcoord;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    int flip;
} ubuf;

layout(binding = 1) uniform sampler2DMS tex;

void main()
{
    ivec2 tc = ivec2(floor(vec2(textureSize(tex)) * v_texcoord));
    vec4 c = texelFetch(tex, tc, 0) + texelFetch(tex, tc, 1) + texelFetch(tex, tc, 2) + texelFetch(tex, tc, 3);
    c /= 4.0;
    fragColor = vec4(c.rgb * c.a, c.a);
}
