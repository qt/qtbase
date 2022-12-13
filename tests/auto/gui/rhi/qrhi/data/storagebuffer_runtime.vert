#version 450

layout (location = 0) in vec3 position;

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

layout (std140, binding = 3) readonly buffer ssbo3
{
    vec4 _vec4;
};

layout (std430, binding = 4) readonly buffer ssbo1
{
    bool _bool[];
};

layout (std430, binding = 1) readonly buffer unused1
{
    int unused[];
}u1;


void main()
{

    // some OpenGL implementations will optimize out the buffer variables if we don't use them
    // resulting in a .length() of 0
    float a = _float[0] == 0 && _bool[0] ? 1 : 1;

    gl_Position = vec4(0);

    if(_bool.length() == 32)
        gl_Position =  a * matrix * vec4(position*_vec4.xyz, _float.length() == 64 ? 1.0 : 0.0);

}
