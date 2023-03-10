#version 440

layout(triangles, fractional_odd_spacing, ccw) in;

layout(location = 0) in vec2 in_uv[];
layout(location = 1) in vec3 in_normal[];

//layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float displacementAmount;
    float tessInner;
    float tessOuter;
    int useTex;
};

layout(binding = 1) uniform sampler2D displacementMap;

void main()
{
    vec4 pos = (gl_TessCoord.x * gl_in[0].gl_Position) + (gl_TessCoord.y * gl_in[1].gl_Position) + (gl_TessCoord.z * gl_in[2].gl_Position);
    vec2 uv = gl_TessCoord.x * in_uv[0].xy + gl_TessCoord.y * in_uv[1].xy;
    vec3 normal = normalize(gl_TessCoord.x * in_normal[0] + gl_TessCoord.y * in_normal[1] + gl_TessCoord.z * in_normal[2]);

    vec4 c = texture(displacementMap, uv);
    const vec3 yCoeff_709 = vec3(0.2126, 0.7152, 0.0722);
    float df = dot(c.rgb, yCoeff_709);
    if (useTex == 0)
        df = 1.0;
    vec3 displacedPos = pos.xyz + normal * df * displacementAmount;
    gl_Position = mvp * vec4(displacedPos, 1.0);

//    out_uv = uv;
    out_normal = normal;
}
