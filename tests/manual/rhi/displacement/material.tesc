#version 440

layout(vertices = 3) out;

layout(location = 0) in vec2 in_uv[];
layout(location = 1) in vec3 in_normal[];

layout(location = 0) out vec2 out_uv[];
layout(location = 1) out vec3 out_normal[];

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float displacementAmount;
    float tessInner;
    float tessOuter;
    int useTex;
};

void main()
{
    if (gl_InvocationID == 0) {
        gl_TessLevelOuter[0] = tessOuter;
        gl_TessLevelOuter[1] = tessOuter;
        gl_TessLevelOuter[2] = tessOuter;

        gl_TessLevelInner[0] = tessInner;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    out_uv[gl_InvocationID] = in_uv[gl_InvocationID];
    out_normal[gl_InvocationID] = in_normal[gl_InvocationID];
}
