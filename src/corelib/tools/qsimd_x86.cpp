// This is a generated file. DO NOT EDIT.
// Please see util/x86simdgen/generate.pl
#include "qsimd_p.h"

static const char features_string[] =
    " sse2\0"
    " sse3\0"
    " ssse3\0"
    " fma\0"
    " sse4.1\0"
    " sse4.2\0"
    " movbe\0"
    " popcnt\0"
    " aes\0"
    " avx\0"
    " f16c\0"
    " rdrnd\0"
    " bmi\0"
    " hle\0"
    " avx2\0"
    " bmi2\0"
    " rtm\0"
    " avx512f\0"
    " avx512dq\0"
    " rdseed\0"
    " avx512ifma\0"
    " avx512pf\0"
    " avx512er\0"
    " avx512cd\0"
    " sha\0"
    " avx512bw\0"
    " avx512vl\0"
    " avx512vbmi\0"
    " avx512vbmi2\0"
    " gfni\0"
    " vaes\0"
    " avx512vnni\0"
    " avx512bitalg\0"
    " avx512vpopcntdq\0"
    " avx5124nniw\0"
    " avx5124fmaps\0"
    "\0";

static const quint16 features_indices[] = {
    306,   0,   6,  12,  19,  24,  32,  40,
     47,  55,  60,  65,  71,  78,  83,  88,
     94, 100, 105, 114, 124, 132, 144, 154,
    164, 174, 179, 189, 199, 211, 224, 230,
    236, 248, 262, 279, 292
};

enum X86CpuidLeaves {
    Leaf1ECX,
    Leaf1EDX,
    Leaf7_0EBX,
    Leaf7_0ECX,
    Leaf7_0EDX,
    X86CpuidMaxLeaf
};

static const quint8 x86_locators[] = {
    Leaf1EDX*32 + 26, // sse2
    Leaf1ECX*32 +  0, // sse3
    Leaf1ECX*32 +  9, // ssse3
    Leaf1ECX*32 + 12, // fma
    Leaf1ECX*32 + 19, // sse4.1
    Leaf1ECX*32 + 20, // sse4.2
    Leaf1ECX*32 + 22, // movbe
    Leaf1ECX*32 + 23, // popcnt
    Leaf1ECX*32 + 25, // aes
    Leaf1ECX*32 + 28, // avx
    Leaf1ECX*32 + 29, // f16c
    Leaf1ECX*32 + 30, // rdrnd
    Leaf7_0EBX*32 +  3, // bmi
    Leaf7_0EBX*32 +  4, // hle
    Leaf7_0EBX*32 +  5, // avx2
    Leaf7_0EBX*32 +  8, // bmi2
    Leaf7_0EBX*32 + 11, // rtm
    Leaf7_0EBX*32 + 16, // avx512f
    Leaf7_0EBX*32 + 17, // avx512dq
    Leaf7_0EBX*32 + 18, // rdseed
    Leaf7_0EBX*32 + 21, // avx512ifma
    Leaf7_0EBX*32 + 26, // avx512pf
    Leaf7_0EBX*32 + 27, // avx512er
    Leaf7_0EBX*32 + 28, // avx512cd
    Leaf7_0EBX*32 + 29, // sha
    Leaf7_0EBX*32 + 30, // avx512bw
    Leaf7_0EBX*32 + 31, // avx512vl
    Leaf7_0ECX*32 +  1, // avx512vbmi
    Leaf7_0ECX*32 +  6, // avx512vbmi2
    Leaf7_0ECX*32 +  8, // gfni
    Leaf7_0ECX*32 +  9, // vaes
    Leaf7_0ECX*32 + 11, // avx512vnni
    Leaf7_0ECX*32 + 12, // avx512bitalg
    Leaf7_0ECX*32 + 14, // avx512vpopcntdq
    Leaf7_0EDX*32 +  2, // avx5124nniw
    Leaf7_0EDX*32 +  3  // avx5124fmaps
};

// List of AVX512 features (see detectProcessorFeatures())
static const quint64 AllAVX512 = 0
        | CpuFeatureAVX512F
        | CpuFeatureAVX512DQ
        | CpuFeatureAVX512IFMA
        | CpuFeatureAVX512PF
        | CpuFeatureAVX512ER
        | CpuFeatureAVX512CD
        | CpuFeatureAVX512BW
        | CpuFeatureAVX512VL
        | CpuFeatureAVX512VBMI
        | CpuFeatureAVX512VBMI2
        | CpuFeatureAVX512VNNI
        | CpuFeatureAVX512BITALG
        | CpuFeatureAVX512VPOPCNTDQ
        | CpuFeatureAVX5124NNIW
        | CpuFeatureAVX5124FMAPS;
