#version 430

# define M_PI 3.14159265358979323846

layout(points) in;
layout(line_strip, max_vertices = 7) out;

layout(std140, binding = 0) uniform buf {
    float radius;
};

void main(void)
{

    for(int i=0;i<7;++i)
    {
        float theta = float(i) / 6.0f * 2.0 * M_PI;

        gl_Position = gl_in[0].gl_Position;
        gl_Position.xy += radius * vec2(cos(theta), sin(theta));

        EmitVertex();
    }
    EndPrimitive();

}
