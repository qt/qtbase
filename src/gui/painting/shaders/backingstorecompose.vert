#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

layout(location = 0) out vec2 v_texcoord;

layout(std140, binding = 0) uniform buf {
    mat4 vertexTransform;
    mat3 textureTransform;
    float opacity;
    int textureSwizzle;
};

void main()
{
    v_texcoord = (textureTransform * vec3(texcoord, 1.0)).xy;
    gl_Position = vertexTransform * vec4(position, 1.0);
}
