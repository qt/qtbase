#version 440

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;

layout(binding = 0, r32ui) uniform uimage2D img;

void main()
{
    imageStore(img, ivec2(gl_FragCoord.xy), uvec4(0x1234u));
    fragColor = vec4(0.0);
}
