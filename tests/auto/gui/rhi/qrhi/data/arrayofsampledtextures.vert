#version 440

// Same as simpletextured.vert, but built for SM 6.0 as well, to go with
// arrayofsampledtextures.frag: with D3D12 all stages in a pipeline have to be
// either DXBC or DXIL, mixing the two is not accepted.

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;

layout(location = 0) out vec2 uv;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    uv = texcoord;
    gl_Position = position;
}
