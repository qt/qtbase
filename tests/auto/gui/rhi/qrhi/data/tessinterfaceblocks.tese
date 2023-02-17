#version 440

layout(triangles, fractional_odd_spacing, ccw) in;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

layout(location = 5) in TescOutA {
    vec3 color;
    int id;
}tcOutA[];

layout(location = 10) in TescOutB {
    vec2 some;
    int other[3];
    vec3 variables;
}tcOutB[];

layout(location = 2) patch in TescOutC {
    vec3 stuff;
    float more_stuff;
}tcOutC;

layout(location = 0) out vec3 outColor;

struct A {
    vec3 color;
    int id;
};

struct B {
    vec2 some;
    int other[3];
    vec3 variables;
};

struct C {
    vec3 stuff;
    float more_stuff;
};

struct Element {
    A a[3];
    B b[3];
    C c;
    vec4 tesslevelOuter;
    vec2 tessLevelInner;
    float pointSize[3];
    float clipDistance[3][5];
    vec3 tessCoord;
    int patchVerticesIn;
    int primitiveID;
};

layout(std430, binding = 1) buffer result {
    int count;
    Element elements[];
};

void main()
{
    gl_Position = mvp * ((gl_TessCoord.x * gl_in[0].gl_Position) + (gl_TessCoord.y * gl_in[1].gl_Position) + (gl_TessCoord.z * gl_in[2].gl_Position));
    outColor = gl_TessCoord.x * tcOutA[0].color + gl_TessCoord.y * tcOutA[1].color + gl_TessCoord.z * tcOutA[2].color;

    count = 1;

    elements[gl_PrimitiveID].c.stuff = tcOutC.stuff;
    elements[gl_PrimitiveID].c.more_stuff = tcOutC.more_stuff;
    elements[gl_PrimitiveID].tesslevelOuter = vec4(gl_TessLevelOuter[0], gl_TessLevelOuter[1], gl_TessLevelOuter[2], gl_TessLevelOuter[3]);
    elements[gl_PrimitiveID].tessLevelInner = vec2(gl_TessLevelInner[0], gl_TessLevelInner[1]);

    for (int i = 0; i < 3; ++i) {

        elements[gl_PrimitiveID].a[i].color = tcOutA[i].color;
        elements[gl_PrimitiveID].a[i].id = tcOutA[i].id;

        elements[gl_PrimitiveID].b[i].some = tcOutB[i].some;
        elements[gl_PrimitiveID].b[i].other = tcOutB[i].other;
        elements[gl_PrimitiveID].b[i].variables = tcOutB[i].variables;

        elements[gl_PrimitiveID].pointSize[i] = gl_in[i].gl_PointSize;
        elements[gl_PrimitiveID].clipDistance[i][0] = gl_in[i].gl_ClipDistance[0];
        elements[gl_PrimitiveID].clipDistance[i][1] = gl_in[i].gl_ClipDistance[1];
        elements[gl_PrimitiveID].clipDistance[i][2] = gl_in[i].gl_ClipDistance[2];
        elements[gl_PrimitiveID].clipDistance[i][3] = gl_in[i].gl_ClipDistance[3];
        elements[gl_PrimitiveID].clipDistance[i][4] = gl_in[i].gl_ClipDistance[4];

    }

    elements[gl_PrimitiveID].tessCoord = gl_TessCoord;
    elements[gl_PrimitiveID].patchVerticesIn = 3;
    elements[gl_PrimitiveID].primitiveID = gl_PrimitiveID;

}

