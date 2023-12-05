#version 440
#extension GL_EXT_multiview : require

layout(location = 0) in vec4 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 v_color;

layout(std140, binding = 0) uniform buf
{
    mat4 mvp[2];
};

void main()
{
    v_color = color;
    gl_Position = mvp[gl_ViewIndex] * pos;
}
