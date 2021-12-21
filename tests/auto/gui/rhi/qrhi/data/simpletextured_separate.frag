#version 440

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(binding = 3) uniform texture2D tex;
layout(binding = 5) uniform sampler samp;

void main()
{
    vec4 c = texture(sampler2D(tex, samp), uv);
    c.rgb *= c.a;
    fragColor = c;
}
