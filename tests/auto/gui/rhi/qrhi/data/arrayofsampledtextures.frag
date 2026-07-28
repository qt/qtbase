#version 440

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D tex[8];

// This one is built for SM 6.0 in addition to SM 5.0 (see buildshaders.bat).
// SM 5.0 has no descriptor ranges, so it is only with 6.0 that an array of
// textures/samplers appears as a single shader-declared range, which the D3D12
// root signature then has to match.
//
// Splits the quad into 8 vertical columns, each showing one of the 8 textures.
// The array is indexed with literals only, because dynamically indexing an
// array of samplers is not available in all the shading language versions this
// gets translated to.
void main()
{
    int idx = int(clamp(floor(uv.x * 8.0), 0.0, 7.0));
    vec4 c;
    if (idx == 0)
        c = texture(tex[0], uv);
    else if (idx == 1)
        c = texture(tex[1], uv);
    else if (idx == 2)
        c = texture(tex[2], uv);
    else if (idx == 3)
        c = texture(tex[3], uv);
    else if (idx == 4)
        c = texture(tex[4], uv);
    else if (idx == 5)
        c = texture(tex[5], uv);
    else if (idx == 6)
        c = texture(tex[6], uv);
    else
        c = texture(tex[7], uv);
    fragColor = c;
}
