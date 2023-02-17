#version 440

layout(vertices = 3) out;

layout(location = 4) in VertOut
{
    vec3 v_color;
    int a;
    float b;
}vOut[];

layout(location = 5) out TescOutA {
    vec3 color;
    int id;
}tcOutA[];

layout(location = 10) out TescOutB {
    vec2 some;
    int other[3];
    vec3 variables;
}tcOutB[];

layout(location = 2) patch out TescOutC {
    vec3 stuff;
    float more_stuff;
}tcOutC;

void main()
{
    // tesc builtin outputs
    gl_TessLevelOuter[0] = 1.0;
    gl_TessLevelOuter[1] = 2.0;
    gl_TessLevelOuter[2] = 3.0;
    gl_TessLevelOuter[3] = 4.0;
    gl_TessLevelInner[0] = 5.0;
    gl_TessLevelInner[1] = 6.0;

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    gl_out[gl_InvocationID].gl_PointSize = 10 + gl_InvocationID;
    gl_out[gl_InvocationID].gl_ClipDistance[0] = 20.0 + gl_InvocationID;
    gl_out[gl_InvocationID].gl_ClipDistance[1] = 40.0 + gl_InvocationID;
    gl_out[gl_InvocationID].gl_ClipDistance[2] = 60.0 + gl_InvocationID;
    gl_out[gl_InvocationID].gl_ClipDistance[3] = 80.0 + gl_InvocationID;
    gl_out[gl_InvocationID].gl_ClipDistance[4] = 100.0 + gl_InvocationID;

    // outputs
    tcOutA[gl_InvocationID].color = vOut[gl_InvocationID].v_color;
    tcOutA[gl_InvocationID].id = gl_InvocationID + 91;
    tcOutB[gl_InvocationID].some = vec2(gl_InvocationID, vOut[gl_InvocationID].a);
    tcOutB[gl_InvocationID].other[0] = gl_PrimitiveID + 10;
    tcOutB[gl_InvocationID].other[1] = gl_PrimitiveID + 20;
    tcOutB[gl_InvocationID].other[2] = gl_PrimitiveID + 30;
    tcOutB[gl_InvocationID].variables = vec3(3.0f, vOut[gl_InvocationID].b, 17.0f);
    tcOutC.stuff = vec3(1.0, 2.0, 3.0);
    tcOutC.more_stuff = 4.0;
}
