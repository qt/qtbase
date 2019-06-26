#version 440

layout(location = 0) in vec3 v_coord;
layout(location = 0) out vec4 fragColor;
layout(binding = 1) uniform samplerCube tex;

void main()
{
    fragColor = vec4(texture(tex, v_coord).rgb, 1.0);
}
