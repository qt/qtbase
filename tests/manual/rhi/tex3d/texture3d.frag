#version 440

layout(location = 0) in vec2 v_texcoord;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    int flip;
    int idx;
} ubuf;

layout(binding = 1) uniform sampler3D tex;

void main()
{
    float w = 0.0;
    if (ubuf.idx == 1)
        w = 0.5;
    else if (ubuf.idx == 2)
        w = 1.0;
    vec4 c = texture(tex, vec3(v_texcoord, w));
    fragColor = vec4(c.rgb * c.a, c.a);
}
