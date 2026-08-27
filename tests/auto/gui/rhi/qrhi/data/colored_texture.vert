#version 440
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;

layout(std140, binding = 0) uniform Ubuf {
    mat4 mvp;
} ubuf;

layout(binding = 1) uniform sampler2D tex;

layout(location = 0) out vec4 vColor;

void main()
{
    gl_Position = ubuf.mvp * vec4(inPos, 1.0);
    // The sampling is the point of this shader: it makes the vertex stage
    // reach a texture, and so need an argument buffer with Metal.
    vColor = inColor * textureLod(tex, vec2(0.5), 0.0);
}
