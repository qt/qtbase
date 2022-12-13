#version 450

layout (location = 0) out vec4 fragColor;

layout (std430, binding = 1) readonly buffer ssboG
{
    float g[];
};

layout (std430, binding = 2) readonly buffer ssboB
{
    float b[];
};

layout (std430, binding = 6) readonly buffer ssboR
{
    float r[];
};

layout (std430, binding = 3) readonly buffer ssbo3
{
    vec4 _vec4;
};

void main()
{

    // some OpenGL implementations will optimize out the buffer variables if we don't use them
    // resulting in a .length() of 0.
    float a = (r[0]+g[0]+b[0])>0?1:1;

    fragColor = a * vec4(r.length(), g.length(), b.length(), 255)/vec4(255);
}
