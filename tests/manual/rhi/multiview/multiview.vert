#version 440
#extension GL_EXT_multiview : require

layout(location = 0) in vec4 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 translation;

layout(location = 0) out vec3 v_color;
layout(location = 1) out flat uint v_viewIndex;

layout(std140, binding = 0) uniform buf
{
    mat4 mvp[2];
};

void main()
{
    v_color = color;
    gl_Position = mvp[gl_ViewIndex] * (pos + vec4(translation, 0.0));
    v_viewIndex = gl_ViewIndex;
}
