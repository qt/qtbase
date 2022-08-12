struct Input
{
    float3 color : TEXCOORD0;
    float4 position : SV_Position;
};

struct Output
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct ConstantData
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

ConstantData patchConstFunc(InputPatch<Input, 3> ip, uint PatchID : SV_PrimitiveID )
{
    ConstantData d;
    d.edges[0] = 4.0;
    d.edges[1] = 4.0;
    d.edges[2] = 4.0;
    d.inside = 4.0;
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
    output.color = patch[pointId].color;
    return output;
}
