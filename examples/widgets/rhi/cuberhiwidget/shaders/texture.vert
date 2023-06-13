#version 440

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;
layout(location = 0) out vec2 v_texcoord;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

void main()
{
    v_texcoord = vec2(texcoord.x, texcoord.y);
    gl_Position = mvp * position;
}
