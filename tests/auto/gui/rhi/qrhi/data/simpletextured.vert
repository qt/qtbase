#version 440

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;

layout(location = 0) out vec2 uv;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    uv = texcoord;
    gl_Position = position;
}
