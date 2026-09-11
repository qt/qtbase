#version 440

layout(location = 0) in vec4 position;

out gl_PerVertex { vec4 gl_Position; };

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
} ubuf;

layout(push_constant) uniform PC {
    vec4 translation;
    vec4 color;
} pc;

void main()
{
    gl_Position = ubuf.mvp * (position + vec4(pc.translation.xyz, 0.0));
}
