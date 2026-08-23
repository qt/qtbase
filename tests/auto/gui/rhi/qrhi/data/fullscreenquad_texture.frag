#version 440

layout(location = 0) in vec2 v_uv;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D tex;

void main()
{
    fragColor = texture(tex, v_uv);
}
