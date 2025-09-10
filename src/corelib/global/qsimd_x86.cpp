// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// This is a generated file. DO NOT EDIT.
// Please see util/x86simdgen/README.md

#include "qsimd_x86_p.h"

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
    " avx2\0"
    " bmi2\0"
    " avx512f\0"
    " avx512dq\0"
    " rdseed\0"
    " avx512ifma\0"
    " avx512cd\0"
    " sha\0"
    " avx512bw\0"
    " avx512vl\0"
    " avx512vbmi\0"
    " waitpkg\0"
    " avx512vbmi2\0"
    " shstk\0"
    " gfni\0"
    " vaes\0"
    " avx512bitalg\0"
    " avx512vpopcntdq\0"
    " hybrid\0"
    " ibt\0"
    " avx512fp16\0"
    " raoint\0"
    " cmpccxadd\0"
    " avxifma\0"
    " lam\0"
    "\0";

static const uint16_t features_indices[] = {
      0,   6,  12,  19,  24,  32,  40,  47,
     55,  60,  65,  71,  78,  83,  89,  95,
    104, 114, 122, 134, 144, 149, 159, 169,
    181, 190, 203, 210, 216, 222, 236, 253,
    261, 266, 278, 286, 297, 306,
};

enum X86CpuidLeaves {
    Leaf01EDX,
    Leaf01ECX,
    Leaf07_00EBX,
    Leaf07_00ECX,
    Leaf07_00EDX,
    Leaf07_01EAX,
    Leaf07_01EDX,
    Leaf13_01EAX,
    Leaf80000001hECX,
    Leaf80000008hEBX,
    X86CpuidMaxLeaf
};

static const uint16_t x86_locators[] = {
    Leaf01EDX*32 + 26,                // sse2
    Leaf01ECX*32 +  0,                // sse3
    Leaf01ECX*32 +  9,                // ssse3
    Leaf01ECX*32 + 12,                // fma
    Leaf01ECX*32 + 19,                // sse4.1
    Leaf01ECX*32 + 20,                // sse4.2
    Leaf01ECX*32 + 22,                // movbe
    Leaf01ECX*32 + 23,                // popcnt
    Leaf01ECX*32 + 25,                // aes
    Leaf01ECX*32 + 28,                // avx
    Leaf01ECX*32 + 29,                // f16c
    Leaf01ECX*32 + 30,                // rdrnd
    Leaf07_00EBX*32 +  3,             // bmi
    Leaf07_00EBX*32 +  5,             // avx2
    Leaf07_00EBX*32 +  8,             // bmi2
    Leaf07_00EBX*32 + 16,             // avx512f
    Leaf07_00EBX*32 + 17,             // avx512dq
    Leaf07_00EBX*32 + 18,             // rdseed
    Leaf07_00EBX*32 + 21,             // avx512ifma
    Leaf07_00EBX*32 + 28,             // avx512cd
    Leaf07_00EBX*32 + 29,             // sha
    Leaf07_00EBX*32 + 30,             // avx512bw
    Leaf07_00EBX*32 + 31,             // avx512vl
    Leaf07_00ECX*32 +  1,             // avx512vbmi
    Leaf07_00ECX*32 +  5,             // waitpkg
    Leaf07_00ECX*32 +  6,             // avx512vbmi2
    Leaf07_00ECX*32 +  7,             // shstk
    Leaf07_00ECX*32 +  8,             // gfni
    Leaf07_00ECX*32 +  9,             // vaes
    Leaf07_00ECX*32 + 12,             // avx512bitalg
    Leaf07_00ECX*32 + 14,             // avx512vpopcntdq
    Leaf07_00EDX*32 + 15,             // hybrid
    Leaf07_00EDX*32 + 20,             // ibt
    Leaf07_00EDX*32 + 23,             // avx512fp16
    Leaf07_01EAX*32 +  3,             // raoint
    Leaf07_01EAX*32 +  6,             // cmpccxadd
    Leaf07_01EAX*32 + 23,             // avxifma
    Leaf07_01EAX*32 + 26,             // lam
};

struct X86Architecture
{
    uint64_t features;
    char name[17 + 1];
};

static const struct X86Architecture x86_architectures[] = {
    { cpu_core2, "Core2" },
    { cpu_westmere, "Westmere" },
    { cpu_sandybridge, "Sandy Bridge" },
    { cpu_silvermont, "Silvermont" },
    { cpu_ivybridge, "Ivy Bridge" },
    { cpu_goldmont, "Goldmont" },
    { cpu_haswell, "Haswell" },
    { cpu_broadwell, "Broadwell" },
    { cpu_tremont, "Tremont" },
    { cpu_skylake, "Skylake" },
    { cpu_skylake_avx512, "Skylake (Avx512)" },
    { cpu_cascadelake, "Cascade Lake" },
    { cpu_cooperlake, "Cooper Lake" },
    { cpu_cannonlake, "Cannon Lake" },
    { cpu_gracemont, "Gracemont" },
    { cpu_icelake_client, "Ice Lake (Client)" },
    { cpu_icelake_server, "Ice Lake (Server)" },
    { cpu_crestmont, "Crestmont" },
    { cpu_tigerlake, "Tiger Lake" },
    { cpu_clearwaterforest, "Clearwater Forest" },
    { cpu_grandridge, "Grand Ridge" },
    { cpu_raptorcove, "Raptor Cove" },
    { cpu_redwoodcove, "Redwood Cove" },
    { cpu_emeraldrapids, "Emerald Rapids" },
    { cpu_graniterapids, "Granite Rapids" },
};

enum XSaveBits {
    XSave_X87          = 0x0001,            // X87 and MMX state
    XSave_SseState     = 0x0002,            // SSE: 128 bits of XMM registers
    XSave_Ymm_Hi128    = 0x0004,            // AVX: high 128 bits in YMM registers
    XSave_Bndregs      = 0x0008,            // Memory Protection Extensions
    XSave_Bndcsr       = 0x0010,            // Memory Protection Extensions
    XSave_OpMask       = 0x0020,            // AVX512: k0 through k7
    XSave_Zmm_Hi256    = 0x0040,            // AVX512: high 256 bits of ZMM0-15
    XSave_Hi16_Zmm     = 0x0080,            // AVX512: all 512 bits of ZMM16-31
    XSave_PTState      = 0x0100,            // Processor Trace
    XSave_PKRUState    = 0x0200,            // Protection Key
    XSave_CetUState    = 0x0800,            // CET: user mode
    XSave_CetSState    = 0x1000,            // CET: supervisor mode
    XSave_HdcState     = 0x2000,            // Hardware Duty Cycle
    XSave_UintrState   = 0x4000,            // User Interrupts
    XSave_HwpState     = 0x10000,           // Hardware P-State
    XSave_Xtilecfg     = 0x20000,           // AMX: XTILECFG register
    XSave_Xtiledata    = 0x40000,           // AMX: data in the tiles
    XSave_AvxState     = XSave_SseState | XSave_Ymm_Hi128,
    XSave_MPXState     = XSave_Bndregs | XSave_Bndcsr,
    XSave_Avx512State  = XSave_AvxState | XSave_OpMask | XSave_Zmm_Hi256 | XSave_Hi16_Zmm,
    XSave_CetState     = XSave_CetUState | XSave_CetSState,
    XSave_AmxState     = XSave_Xtilecfg | XSave_Xtiledata,
};

// List of features requiring XSave_AvxState
static const uint64_t XSaveReq_AvxState = 0
        | cpu_feature_fma
        | cpu_feature_avx
        | cpu_feature_f16c
        | cpu_feature_avx2
        | cpu_feature_avx512f
        | cpu_feature_avx512dq
        | cpu_feature_avx512ifma
        | cpu_feature_avx512cd
        | cpu_feature_avx512bw
        | cpu_feature_avx512vl
        | cpu_feature_avx512vbmi
        | cpu_feature_avx512vbmi2
        | cpu_feature_vaes
        | cpu_feature_avx512bitalg
        | cpu_feature_avx512vpopcntdq
        | cpu_feature_avx512fp16
        | cpu_feature_avxifma;

// List of features requiring XSave_Avx512State
static const uint64_t XSaveReq_Avx512State = 0
        | cpu_feature_avx512f
        | cpu_feature_avx512dq
        | cpu_feature_avx512ifma
        | cpu_feature_avx512cd
        | cpu_feature_avx512bw
        | cpu_feature_avx512vl
        | cpu_feature_avx512vbmi
        | cpu_feature_avx512vbmi2
        | cpu_feature_avx512bitalg
        | cpu_feature_avx512vpopcntdq
        | cpu_feature_avx512fp16;

// List of features requiring XSave_CetState
static const uint64_t XSaveReq_CetState = 0
        | cpu_feature_shstk;

struct XSaveRequirementMapping
{
    uint64_t cpu_features;
    uint64_t xsave_state;
};

static const struct XSaveRequirementMapping xsave_requirements[] = {
    { XSaveReq_AvxState, XSave_AvxState },
    { XSaveReq_Avx512State, XSave_Avx512State },
    { XSaveReq_CetState, XSave_CetState },
};

