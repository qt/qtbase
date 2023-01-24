struct Input
{
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 position : SV_Position;
};

struct Output
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
};

struct ConstantData
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

cbuffer buf : register(b0)
{
    row_major float4x4 mvp : packoffset(c0);
    float displacementAmount : packoffset(c4);
    float tessInner : packoffset(c4.y);
    float tessOuter : packoffset(c4.z);
    int useTex : packoffset(c4.w);
};

ConstantData patchConstFunc(InputPatch<Input, 3> ip, uint PatchID : SV_PrimitiveID )
{
    ConstantData d;
    d.edges[0] = tessOuter;
    d.edges[1] = tessOuter;
    d.edges[2] = tessOuter;
    d.inside = tessInner;
    return d;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("patchConstFunc")]
Output main(InputPatch<Input, 3> patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    Output output;
    output.position = patch[pointId].position;
    output.uv = patch[pointId].uv;
    output.normal = patch[pointId].normal;
    return output;
}
