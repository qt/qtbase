// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.

RWTexture2D<float4> OutMip1 : register(u0);
RWTexture2D<float4> OutMip2 : register(u1);
RWTexture2D<float4> OutMip3 : register(u2);
RWTexture2D<float4> OutMip4 : register(u3);
Texture2D<float4> SrcMip : register(t0);
SamplerState BilinearClamp : register(s0);

cbuffer CB0 : register(b0)
{
    uint SrcMipLevel;  // Texture level of source mip
    uint NumMipLevels; // Number of OutMips to write: [1, 4]
    float2 TexelSize;  // 1.0 / OutMip1.Dimensions
}

// The reason for separating channels is to reduce bank conflicts in the
// local data memory controller.  A large stride will cause more threads
// to collide on the same memory bank.
groupshared float gs_R[64];
groupshared float gs_G[64];
groupshared float gs_B[64];
groupshared float gs_A[64];

void StoreColor( uint Index, float4 Color )
{
    gs_R[Index] = Color.r;
    gs_G[Index] = Color.g;
    gs_B[Index] = Color.b;
    gs_A[Index] = Color.a;
}

float4 LoadColor( uint Index )
{
    return float4( gs_R[Index], gs_G[Index], gs_B[Index], gs_A[Index]);
}

[numthreads( 8, 8, 1 )]
void csMain( uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID )
{
    // Use 4 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
    // in both directions.
    float2 UV1 = TexelSize * (DTid.xy + float2(0.25, 0.25));
    float2 O = TexelSize * 0.5;
    float4 Src1 = SrcMip.SampleLevel(BilinearClamp, UV1, SrcMipLevel);
    Src1 += SrcMip.SampleLevel(BilinearClamp, UV1 + float2(O.x, 0.0), SrcMipLevel);
    Src1 += SrcMip.SampleLevel(BilinearClamp, UV1 + float2(0.0, O.y), SrcMipLevel);
    Src1 += SrcMip.SampleLevel(BilinearClamp, UV1 + float2(O.x, O.y), SrcMipLevel);
    Src1 *= 0.25;

    OutMip1[DTid.xy] = Src1;

    // A scalar (constant) branch can exit all threads coherently.
    if (NumMipLevels == 1)
        return;

    // Without lane swizzle operations, the only way to share data with other
    // threads is through LDS.
    StoreColor(GI, Src1);

    // This guarantees all LDS writes are complete and that all threads have
    // executed all instructions so far (and therefore have issued their LDS
    // write instructions.)
    GroupMemoryBarrierWithGroupSync();

    // With low three bits for X and high three bits for Y, this bit mask
    // (binary: 001001) checks that X and Y are even.
    if ((GI & 0x9) == 0)
    {
        float4 Src2 = LoadColor(GI + 0x01);
        float4 Src3 = LoadColor(GI + 0x08);
        float4 Src4 = LoadColor(GI + 0x09);
        Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);

        OutMip2[DTid.xy / 2] = Src1;
        StoreColor(GI, Src1);
    }

    if (NumMipLevels == 2)
        return;

    GroupMemoryBarrierWithGroupSync();

    // This bit mask (binary: 011011) checks that X and Y are multiples of four.
    if ((GI & 0x1B) == 0)
    {
        float4 Src2 = LoadColor(GI + 0x02);
        float4 Src3 = LoadColor(GI + 0x10);
        float4 Src4 = LoadColor(GI + 0x12);
        Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);

        OutMip3[DTid.xy / 4] = Src1;
        StoreColor(GI, Src1);
    }

    if (NumMipLevels == 3)
        return;

    GroupMemoryBarrierWithGroupSync();

    // This bit mask would be 111111 (X & Y multiples of 8), but only one
    // thread fits that criteria.
    if (GI == 0)
    {
        float4 Src2 = LoadColor(GI + 0x04);
        float4 Src3 = LoadColor(GI + 0x20);
        float4 Src4 = LoadColor(GI + 0x24);
        Src1 = 0.25 * (Src1 + Src2 + Src3 + Src4);

        OutMip4[DTid.xy / 8] = Src1;
    }
}
