#version 450

layout(vertices = 3) out;


layout (std430, binding = 7) readonly buffer ssbo7
{
    float float7[];
};

layout (std430, binding = 8) readonly buffer ssbo8
{
    float float8[];
};

layout (std430, binding = 9) readonly buffer ssbo9
{
    float float9[];
};

layout (std430, binding = 10) readonly buffer ssbo10
{
    float float10[];
};

void main()
{

    // some OpenGL implementations will optimize out the buffer variables if we don't use them
    // resulting in a .length() of 0
    float a = float7[0] == 0 && float8[0] == 0 && float9[0] == 0 && float10[0] == 0 ? 1 : 1;

    if (gl_InvocationID == 0) {
        gl_TessLevelOuter[0] = float7.length() * a;
        gl_TessLevelOuter[1] = float8.length() * a;
        gl_TessLevelOuter[2] = float9.length() * a;
        gl_TessLevelInner[0] = float10.length() * a;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

}
