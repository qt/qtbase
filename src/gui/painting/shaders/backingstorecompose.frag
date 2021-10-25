#version 440

layout(location = 0) in vec2 v_texcoord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 vertexTransform;
    mat3 textureTransform;
    float opacity;
    int swapRedBlue;
};

layout(binding = 1) uniform sampler2D textureSampler;

void main()
{
    vec4 tmpFragColor = texture(textureSampler, v_texcoord);
    tmpFragColor.a *= opacity;
    if (swapRedBlue == 0)
        fragColor = tmpFragColor;
    else
        fragColor = tmpFragColor.bgra;
}
