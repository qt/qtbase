#version 440
layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex;

void main()
{
    outColor = vColor * texture(tex, vec2(0.5));
}
