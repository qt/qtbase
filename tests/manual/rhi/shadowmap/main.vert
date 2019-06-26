#version 440

layout(location = 0) in vec4 position;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    mat4 lightMvp;
    mat4 shadowBias;
    int useShadow;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

layout(location = 0) out vec4 vLCVertPos;

void main()
{
    vLCVertPos = ubuf.shadowBias * ubuf.lightMvp * position;
    gl_Position = ubuf.mvp * position;
}
