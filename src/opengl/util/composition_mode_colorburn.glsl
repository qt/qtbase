// Dca' = Sca.Da + Dca.Sa <= Sa.Da ?
//        Sca.(1 - Da) + Dca.(1 - Sa)
//        Sa.(Sca.Da + Dca.Sa - Sa.Da)/Sca + Sca.(1 - Da) + Dca.(1 - Sa)
// Da'  = Sa + Da - Sa.Da 
vec4 composite(vec4 src, vec4 dst)
{
    vec4 result;
    result.rgb = mix(src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a),
                     src.a * (src.rgb * dst.a + dst.rgb * src.a - src.a * dst.a) / max(src.rgb, 0.00001) + src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a),
                     step(src.a * dst.a, src.rgb * dst.a + dst.rgb * src.a));
    result.a = src.a + dst.a - src.a * dst.a;
    return result;
}
