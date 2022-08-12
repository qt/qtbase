struct VertexOutput
{
    float4 position : SV_Position;
};

struct PixelInput
{
    float4 position : SV_POSITION;
};

cbuffer buf : register(b0)
{
    float radius : packoffset(c0);
};

[maxvertexcount(7)]
void main(point VertexOutput input[1], inout LineStream<PixelInput> OutputStream)
{
    PixelInput output;
    for (int i = 0; i < 7; ++i) {
        float theta = float(i) / 6.0f * 2.0 * 3.14159265;
        output.position = input[0].position;
        output.position.xy += radius * float2(cos(theta), sin(theta));
        OutputStream.Append(output);
    }
}
