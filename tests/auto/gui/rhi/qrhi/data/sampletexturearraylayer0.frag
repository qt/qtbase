#version 440

layout(location = 0) in vec2 v_uv;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2DArray texArr;

void main()
{
    fragColor = texture(texArr, vec3(v_uv, 0.0));
}
