#version 450

layout(triangles, fractional_odd_spacing, ccw) in;

layout (std140, binding = 6) uniform unused0
{
    int unused;
}u0;

layout (binding = 0) uniform u
{
    mat4 matrix;
};

layout (std430, binding = 5) readonly buffer ssbo5
{
    float _float[];
};

layout (std430, binding = 8) readonly buffer ssbo8
{
    float float8[];
};

layout (std430, binding = 1) readonly buffer unused1
{
    int unused[];
}u1;


void main()
{
    // some OpenGL implementations will optimize out the buffer variables if we don't use them
    // resulting in a .length() of 0
    float a = _float[0] == 0 && float8[0] == 1 ? 1 : 1;

    if(_float.length() == 64)
        gl_Position = a *  matrix * ((gl_TessCoord.x * gl_in[0].gl_Position) + (gl_TessCoord.y * gl_in[1].gl_Position) + (gl_TessCoord.z * gl_in[2].gl_Position)) * (float8.length()==2?1:0);
}
