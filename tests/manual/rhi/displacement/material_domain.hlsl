struct Input
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

struct PatchInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
};

struct PixelInput
{
    //float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 position : SV_POSITION;
};

cbuffer buf : register(b0)
{
    row_major float4x4 mvp : packoffset(c0);
    float displacementAmount : packoffset(c4);
    float tessInner : packoffset(c4.y);
    float tessOuter : packoffset(c4.z);
    int useTex : packoffset(c4.w);
};

Texture2D<float4> tex : register(t1);
SamplerState texsampler : register(s1);

[domain("tri")]
PixelInput main(Input input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<PatchInput, 3> patch)
{
    PixelInput output;

    float3 pos = uvwCoord.x * patch[0].position + uvwCoord.y * patch[1].position + uvwCoord.z * patch[2].position;
    float2 uv =  uvwCoord.x * patch[0].uv + uvwCoord.y * patch[1].uv;
    float3 normal = normalize(uvwCoord.x * patch[0].normal + uvwCoord.y * patch[1].normal + uvwCoord.z * patch[2].normal);

    float4 c = tex.SampleLevel(texsampler, uv, 0.0);
    const float3 yCoeff_709 = float3(0.2126, 0.7152, 0.0722);
    float df = dot(c.rgb, yCoeff_709);
    if (useTex == 0)
        df = 1.0;
    float3 displacedPos = pos + normal * df * displacementAmount;

    output.position = mul(float4(displacedPos, 1.0), mvp);
    //output.uv = uv;
    output.normal = normal;

    return output;
}
