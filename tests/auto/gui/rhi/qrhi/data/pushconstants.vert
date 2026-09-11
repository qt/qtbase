#version 440

layout(location = 0) in vec4 position;

layout(push_constant) uniform PC {
    vec4 offsetScale;
    vec4 color;
} pc;

void main()
{
    gl_Position = vec4(position.xy * pc.offsetScale.zw + pc.offsetScale.xy, 0.0, 1.0);
}
