#version 440

layout(location = 0) in vec4 position;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = position;
}
