#version 440

layout(vertices = 3) out;

layout(location = 0) in vec3 inColor[];
layout(location = 0) out vec3 outColor[];
layout(location = 1) patch out float a_per_patch_output_variable;

void main()
{
    if (gl_InvocationID == 0) {
        gl_TessLevelOuter[0] = 4.0;
        gl_TessLevelOuter[1] = 4.0;
        gl_TessLevelOuter[2] = 4.0;

        gl_TessLevelInner[0] = 4.0;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    outColor[gl_InvocationID] = inColor[gl_InvocationID];
    a_per_patch_output_variable = 1.0;
}
