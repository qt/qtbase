#version 440

layout(location = 0) in vec4 position;

layout(location = 0) out vec4 vColor;

layout(push_constant) uniform PC {
    mat4 camera;
} pc;

// binding 0 is the camera uniform buffer in the srb, unused by this shader
layout(std140, binding = 1) uniform Object {
    vec4 translation; // xyz = position, w = rotation angle around Y
    vec4 color;
} obj;

vec4 transformed(mat4 camera, vec4 translation, vec4 pos)
{
    float s = sin(translation.w);
    float c = cos(translation.w);
    vec3 p = vec3(c * pos.x + s * pos.z, pos.y, -s * pos.x + c * pos.z) + translation.xyz;
    return camera * vec4(p, 1.0);
}

void main()
{
    vColor = obj.color;
    gl_Position = transformed(pc.camera, obj.translation, position);
}
