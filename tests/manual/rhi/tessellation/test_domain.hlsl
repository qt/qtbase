struct Input
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

struct PatchInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct PixelInput
{
    float3 color : TEXCOORD0;
    float4 position : SV_POSITION;
};

cbuffer buf : register(b0)
{
    row_major float4x4 mvp : packoffset(c0);
    float time : packoffset(c4);
    float amplitude : packoffset(c4.y);
};

[domain("tri")]
PixelInput main(Input input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<PatchInput, 3> patch)
{
    PixelInput output;

    float3 vertexPosition = uvwCoord.x * patch[0].position + uvwCoord.y * patch[1].position + uvwCoord.z * patch[2].position;
    output.position = mul(float4(vertexPosition, 1.0f), mvp);
    output.position.x += sin(time + vertexPosition.y) * amplitude;

    output.color = uvwCoord.x * patch[0].color + uvwCoord.y * patch[1].color + uvwCoord.z * patch[2].color;

    return output;
}
