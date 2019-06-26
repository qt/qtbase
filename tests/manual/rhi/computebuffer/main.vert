#version 440

layout(location = 0) in vec4 position;

out gl_PerVertex { vec4 gl_Position; float gl_PointSize; };

void main()
{
    gl_PointSize = 4.0; // required with Vulkan when drawing points
    gl_Position = position;
}
