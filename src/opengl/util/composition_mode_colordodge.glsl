// Dca' = Sca.Da + Dca.Sa <= Sa.Da ?
//        Dca.Sa/(1 - Sca/Sa) + Sca.(1 - Da) + Dca.(1 - Sa) :
//        Sa.Da + Sca.(1 - Da) + Dca.(1 - Sa)
// Da'  = Sa + Da - Sa.Da 
vec4 composite(vec4 src, vec4 dst)
{
    vec4 result;
    vec3 temp = src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a);
    result.rgb = mix(dst.rgb * src.a / max(1.0 - src.rgb / max(src.a, 0.000001), 0.000001) + temp,
                     src.a * dst.a + temp,
                     step(src.a * dst.a, src.rgb * dst.a + dst.rgb * src.a));

    result.a = src.a + dst.a - src.a * dst.a;
    return result;
}
