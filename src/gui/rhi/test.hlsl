struct Input
{
    float4 position : TEXCOORD0;
    float3 color : TEXCOORD1;
};

struct Output
{
    float4 position : SV_Position;
    float3 color : TEXCOORD0;
};

cbuffer buf : register(b0)
{
    row_major float4x4 ubuf_mvp;
};

Output main(Input input)
{
    Output output;
    output.position = mul(input.position, ubuf_mvp);
    output.color = input.color;
    return output;
}
