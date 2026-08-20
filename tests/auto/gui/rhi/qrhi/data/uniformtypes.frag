#version 440

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf
{
    float f;         // offset   0
    vec2 v2;         // offset   8
    vec3 v3;         // offset  16
    vec4 v4;         // offset  32
    int i1;          // offset  48
    ivec2 i2;        // offset  56
    ivec3 i3;        // offset  64
    ivec4 i4;        // offset  80
    mat2 m2;         // offset  96, column stride 16
    mat3 m3;         // offset 128, column stride 16
    mat4 m4;         // offset 176, column stride 16
    float fArr[2];   // offset 240, array stride 16
    vec2 v2Arr[2];   // offset 272, array stride 16
    vec3 v3Arr[2];   // offset 304, array stride 16
    vec4 v4Arr[2];   // offset 336, array stride 16
    int i1Arr[2];    // offset 368, array stride 16
    ivec2 i2Arr[2];  // offset 400, array stride 16
    ivec3 i3Arr[2];  // offset 432, array stride 16
    ivec4 i4Arr[2];  // offset 464, array stride 16
    mat2 m2Arr[2];   // offset 496, array stride 32, column stride 16
    mat3 m3Arr[2];   // offset 560, array stride 48, column stride 16
    mat4 m4Arr[2];   // offset 656, array stride 64, column stride 16
};                   // total size 784

// The expected value of the n-th float and int component, respectively. The
// test generates the same sequences on the CPU side.
float ef(int n) { return float(n) + 0.5; }
int ei(int n) { return -n; }

int checkIndex = 0;
int firstFailure = 0;

void check(bool ok)
{
    checkIndex += 1;
    if (!ok && firstFailure == 0)
        firstFailure = checkIndex;
}

// Consecutive expected values differ by 1.0, so this is nowhere near lenient
// enough to let a misplaced value slip through.
void checkFloat(float v, int n) { check(abs(v - ef(n)) < 0.01); }

void main()
{
    checkFloat(f, 1);

    for (int k = 0; k < 2; ++k)
        checkFloat(v2[k], 2 + k);

    for (int k = 0; k < 3; ++k)
        checkFloat(v3[k], 4 + k);

    for (int k = 0; k < 4; ++k)
        checkFloat(v4[k], 7 + k);

    for (int c = 0; c < 2; ++c)
        for (int r = 0; r < 2; ++r)
            checkFloat(m2[c][r], 11 + c * 2 + r);

    for (int c = 0; c < 3; ++c)
        for (int r = 0; r < 3; ++r)
            checkFloat(m3[c][r], 15 + c * 3 + r);

    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            checkFloat(m4[c][r], 24 + c * 4 + r);

    for (int e = 0; e < 2; ++e)
        checkFloat(fArr[e], 40 + e);

    for (int e = 0; e < 2; ++e)
        for (int k = 0; k < 2; ++k)
            checkFloat(v2Arr[e][k], 42 + e * 2 + k);

    for (int e = 0; e < 2; ++e)
        for (int k = 0; k < 3; ++k)
            checkFloat(v3Arr[e][k], 46 + e * 3 + k);

    for (int e = 0; e < 2; ++e)
        for (int k = 0; k < 4; ++k)
            checkFloat(v4Arr[e][k], 52 + e * 4 + k);

    for (int e = 0; e < 2; ++e)
        for (int c = 0; c < 2; ++c)
            for (int r = 0; r < 2; ++r)
                checkFloat(m2Arr[e][c][r], 60 + e * 4 + c * 2 + r);

    for (int e = 0; e < 2; ++e)
        for (int c = 0; c < 3; ++c)
            for (int r = 0; r < 3; ++r)
                checkFloat(m3Arr[e][c][r], 68 + e * 9 + c * 3 + r);

    for (int e = 0; e < 2; ++e)
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                checkFloat(m4Arr[e][c][r], 86 + e * 16 + c * 4 + r);

    check(i1 == ei(1));

    for (int k = 0; k < 2; ++k)
        check(i2[k] == ei(2 + k));

    for (int k = 0; k < 3; ++k)
        check(i3[k] == ei(4 + k));

    for (int k = 0; k < 4; ++k)
        check(i4[k] == ei(7 + k));

    for (int e = 0; e < 2; ++e)
        check(i1Arr[e] == ei(11 + e));

    for (int e = 0; e < 2; ++e)
        for (int k = 0; k < 2; ++k)
            check(i2Arr[e][k] == ei(13 + e * 2 + k));

    for (int e = 0; e < 2; ++e)
        for (int k = 0; k < 3; ++k)
            check(i3Arr[e][k] == ei(17 + e * 3 + k));

    for (int e = 0; e < 2; ++e)
        for (int k = 0; k < 4; ++k)
            check(i4Arr[e][k] == ei(23 + e * 4 + k));

    // Red is 0 when everything matched, otherwise it is the 1-based index of
    // the first check that failed.
    fragColor = vec4(float(firstFailure) / 255.0, 0.0, 0.0, 1.0);
}
