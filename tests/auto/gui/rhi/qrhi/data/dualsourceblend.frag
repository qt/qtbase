#version 440

layout(location = 0, index = 0) out vec4 src0;
layout(location = 0, index = 1) out vec4 src1;

void main()
{
    src0 = vec4(1.0, 0.0, 0.25, 0.75);
    src1 = vec4(0.25, 0.5, 0.75, 0.25);
}
