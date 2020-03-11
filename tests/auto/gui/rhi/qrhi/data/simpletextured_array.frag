#version 440

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D tex[3];

void main()
{
    vec4 c0 = texture(tex[0], uv);
    vec4 c1 = texture(tex[1], uv);
    vec4 c2 = texture(tex[2], uv);
    vec4 cc = c0 + c1 + c2;
    vec4 c = vec4(clamp(cc.r, 0.0, 1.0), clamp(cc.g, 0.0, 1.0), clamp(cc.b, 0.0, 1.0), clamp(cc.a, 0.0, 1.0));
    c.rgb *= c.a;
    fragColor = c;
}
