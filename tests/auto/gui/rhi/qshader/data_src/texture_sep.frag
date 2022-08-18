#version 440

layout(location = 0) in vec2 v_texcoord;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D combinedTexSampler;
layout(binding = 2) uniform texture2D sepTex;
layout(binding = 3) uniform sampler sepSampler;
layout(binding = 4) uniform sampler sepSampler2;

void main()
{
    fragColor = texture(sampler2D(sepTex, sepSampler), v_texcoord);
    fragColor *= texture(sampler2D(sepTex, sepSampler2), v_texcoord);
    fragColor *= texture(combinedTexSampler, v_texcoord);
}
