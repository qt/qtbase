#version 440

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;

layout(binding = 0, r32ui) uniform uimage2D img;

void main()
{
    uint value = imageLoad(img, ivec2(gl_FragCoord.xy)).x;
    fragColor = value == 0x1234u ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);
}
