#version 440

layout(location = 0) out vec4 fragColor;

// The unsigned integer counterpart of uniformtypes.frag. Separate because uint
// and uvecN require GLSL 1.30 / 3.00 es and glUniform*ui(), so this cannot be
// targeted at GLSL 120 and 100 es.

layout(std140, binding = 0) uniform buf
{
    uint u1;         // offset   0
    uvec2 u2;        // offset   8
    uvec3 u3;        // offset  16
    uvec4 u4;        // offset  32
    uint u1Arr[2];   // offset  48, array stride 16
    uvec2 u2Arr[2];  // offset  80, array stride 16
    uvec3 u3Arr[2];  // offset 112, array stride 16
    uvec4 u4Arr[2];  // offset 144, array stride 16
};                   // total size 176

// The expected value of the n-th uint component. The test generates the same
// sequence on the CPU side.
uint eu(int n) { return uint(n); }

int checkIndex = 0;
int firstFailure = 0;

void check(bool ok)
{
    checkIndex += 1;
    if (!ok && firstFailure == 0)
        firstFailure = checkIndex;
}

void main()
{
    check(u1 == eu(1));

    for (int k = 0; k < 2; ++k)
        check(u2[k] == eu(2 + k));

    for (int k = 0; k < 3; ++k)
        check(u3[k] == eu(4 + k));

    for (int k = 0; k < 4; ++k)
        check(u4[k] == eu(7 + k));

    for (int e = 0; e < 2; ++e)
        check(u1Arr[e] == eu(11 + e));

    for (int e = 0; e < 2; ++e)
        for (int k = 0; k < 2; ++k)
            check(u2Arr[e][k] == eu(13 + e * 2 + k));

    for (int e = 0; e < 2; ++e)
        for (int k = 0; k < 3; ++k)
            check(u3Arr[e][k] == eu(17 + e * 3 + k));

    for (int e = 0; e < 2; ++e)
        for (int k = 0; k < 4; ++k)
            check(u4Arr[e][k] == eu(23 + e * 4 + k));

    // Red is 0 when everything matched, otherwise it is the 1-based index of
    // the first check that failed.
    fragColor = vec4(float(firstFailure) / 255.0, 0.0, 0.0, 1.0);
}
