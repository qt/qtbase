// Dca' = Sca + Dca - 2.min(Sca.Da, Dca.Sa)
// Da'  = Sa + Da - Sa.Da
vec4 composite(vec4 src, vec4 dst)
{
    vec4 result;
    result.rgb = src.rgb + dst.rgb - 2.0 * min(src.rgb * dst.a, dst.rgb * src.a);
    result.a = src.a + dst.a - src.a * dst.a;
    return result;
}
