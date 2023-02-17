#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;


layout(location = 4) out VertOut
{
    vec3 v_color;
    int a;
    float b;
};

void main()
{
    gl_Position = vec4(position, 1.0);
    v_color = color;
    a = gl_VertexIndex;
    b = 13.0f + gl_VertexIndex;
}
