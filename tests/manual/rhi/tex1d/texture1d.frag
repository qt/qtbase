#version 440

layout(location = 0) in vec2 v_texcoord;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    int idx;
} ubuf;

layout(binding = 1) uniform sampler1D tex;
layout(binding = 2) uniform sampler1DArray texArray;
layout(binding = 3) uniform sampler1D texA;
layout(binding = 4) uniform sampler1DArray texArrayA;
layout(binding = 5) uniform sampler1D texB;
layout(binding = 6) uniform sampler1D texC;
layout(binding = 7) uniform sampler1DArray texArrayB;

void main()
{
    vec4 c = vec4(v_texcoord, 0, 1);

    vec2 coord = vec2(v_texcoord.x, floor(v_texcoord.y*2));

    switch((ubuf.idx/(60*2))%7) {
    case 0:
        c = textureLod(tex, v_texcoord.x, float((ubuf.idx/30)%4));
        break;
    case 1:
        c = textureLod(texArray, coord, float((ubuf.idx/30)%4));
        break;
    case 2:
        c = textureLod(texA, v_texcoord.x, float((ubuf.idx/30)%4));
        break;
    case 3:
        c = textureLod(texArrayA, coord, float((ubuf.idx/30)%4));
        break;
    case 4:
        c = texture(texB, v_texcoord.x);
        break;
    case 5:
        c = texture(texC, v_texcoord.x);
        break;
    case 6:
        c = texture(texArrayB, coord);
        break;
    }

    fragColor = vec4(c.rgb*c.a, c.a);

}
