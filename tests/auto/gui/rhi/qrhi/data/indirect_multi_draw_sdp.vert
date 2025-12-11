#version 460

layout(location = 0) in vec4 vertexPosition;

layout(location = 0) out vec4 vertexColor;

layout(std140, binding = 0) uniform buf
{
    vec4 quadColors[2];
    mat4 quadMvps[2];
} ubuf;

void main()
{
    int drawId = gl_DrawID;
    bool ok = gl_BaseVertex == 0 && gl_BaseInstance == 0;

    vertexColor = ok ? ubuf.quadColors[drawId]
                     : vec4(0.0, 0.0, 1.0, 1.0);

    gl_Position = ubuf.quadMvps[drawId] * vertexPosition;
}
