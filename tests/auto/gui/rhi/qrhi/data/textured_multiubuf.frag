#version 440

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 1) uniform buf {
    float opacity;
} fubuf;

layout(binding = 2) uniform sampler2D tex;

void main()
{
    vec4 c = texture(tex, uv);
    c.a *= fubuf.opacity;
    c.rgb *= c.a;
    fragColor = c;
}
