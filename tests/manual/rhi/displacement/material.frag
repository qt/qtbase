#version 440

layout(location = 0) out vec4 fragColor;

layout(location = 1) in vec3 in_normal;

void main()
{
    fragColor = vec4((normalize(in_normal) + 1.0) / 2.0, 1.0);
}
