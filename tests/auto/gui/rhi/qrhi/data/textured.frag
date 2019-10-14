#version 440

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    float opacity;
} ubuf;

layout(binding = 1) uniform sampler2D tex;

void main()
{
    vec4 c = texture(tex, uv);
    c.a *= ubuf.opacity;
    c.rgb *= c.a;
    fragColor = c;
}
