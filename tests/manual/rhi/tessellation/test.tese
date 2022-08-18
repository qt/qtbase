#version 440

layout(triangles, fractional_odd_spacing, ccw) in;

layout(location = 0) in vec3 inColor[];

layout(location = 0) out vec3 outColor;

// these serve no purpose, just exist to test per-patch outputs
layout(location = 1) patch in vec3 stuff;
layout(location = 2) patch in float more_stuff;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float time;
    float amplitude;
};

void main()
{
    vec4 pos = (gl_TessCoord.x * gl_in[0].gl_Position) + (gl_TessCoord.y * gl_in[1].gl_Position) + (gl_TessCoord.z * gl_in[2].gl_Position);
    gl_Position = mvp * pos;
    gl_Position.x += sin(time + pos.y) * amplitude;
    outColor = gl_TessCoord.x * inColor[0] + gl_TessCoord.y * inColor[1] + gl_TessCoord.z * inColor[2]
    // these are all 1.0, just here to exercise the shader generation and the runtime pipeline setup
        * stuff.x * more_stuff * (gl_TessLevelOuter[0] / 4.0) * (gl_TessLevelInner[0] / 4.0);
}
