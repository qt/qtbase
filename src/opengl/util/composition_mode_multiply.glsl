// Dca' = Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
// Da'  = Sa + Da - Sa.Da 
vec4 composite(vec4 src, vec4 dst)
{
    vec4 result;
    result.rgb = src.rgb * dst.rgb + src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a);
    result.a = src.a + dst.a - src.a * dst.a;
    return result;
}
