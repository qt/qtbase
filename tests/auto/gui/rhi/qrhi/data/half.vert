#version 440

layout(location = 0) in vec3 position;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = vec4(position, 1.0);
}
