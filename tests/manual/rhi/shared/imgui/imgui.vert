#version 440

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec4 color;

layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out vec4 v_color;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float opacity;
    float sdrMult;
};

void main()
{
    v_texcoord = texcoord;
    v_color = color;
    gl_Position = mvp * vec4(position.xy, 0.0, 1.0);
}
