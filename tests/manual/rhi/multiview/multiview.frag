#version 440

layout(location = 0) in vec3 v_color;
layout(location = 1) in flat uint v_viewIndex;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(v_color + v_viewIndex * 0.5, 1.0);
}
