#version 440

layout(location = 0) in vec4 position;

out gl_PerVertex { vec4 gl_Position; };

layout(push_constant) uniform PC {
    mat4 mvp;
} pc;

void main()
{
    gl_Position = pc.mvp * position;
}
