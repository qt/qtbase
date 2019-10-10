#version 440

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D tex;

void main()
{
    vec4 c = texture(tex, uv);
    c.rgb *= c.a;
    fragColor = c;
}
