#version 440

layout(triangles, fractional_odd_spacing, ccw) in;

layout(location = 0) in vec3 inColor[];
layout(location = 0) out vec3 outColor;
layout(location = 1) patch in float a_per_patch_output_variable;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

void main()
{
    gl_Position = mvp * ((gl_TessCoord.x * gl_in[0].gl_Position) + (gl_TessCoord.y * gl_in[1].gl_Position) + (gl_TessCoord.z * gl_in[2].gl_Position));
    outColor = gl_TessCoord.x * inColor[0] + gl_TessCoord.y * inColor[1] + gl_TessCoord.z * inColor[2] * a_per_patch_output_variable;
}
