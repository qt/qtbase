// if 2.Sca <= Sa
//     Dca' = Dca.(Sa + (2.Sca - Sa).(1 - Dca/Da)) + Sca.(1 - Da) + Dca.(1 - Sa)
// otherwise if 2.Sca > Sa and 4.Dca <= Da
//     Dca' = Dca.Sa + Da.(2.Sca - Sa).(4.Dca/Da.(4.Dca/Da + 1).(Dca/Da - 1) + 7.Dca/Da) + Sca.(1 - Da) + Dca.(1 - Sa)
// otherwise if 2.Sca > Sa and 4.Dca > Da
//     Dca' = Dca.Sa + Da.(2.Sca - Sa).((Dca/Da)^0.5 - Dca/Da) + Sca.(1 - Da) + Dca.(1 - Sa)
// Da'  = Sa + Da - Sa.Da 

vec4 composite(vec4 src, vec4 dst)
{
    vec4 result;
    float da = max(dst.a, 0.00001);
    vec3 dst_np = dst.rgb / da;
    result.rgb = mix(dst.rgb * (src.a + (2.0 * src.rgb - src.a) * (1.0 - dst_np)),
                     mix(dst.rgb * src.a + dst.a * (2.0 * src.rgb - src.a) * ((16.0 * dst_np - 12.0) * dst_np + 3.0) * dst_np,
                         dst.rgb * src.a + dst.a * (2.0 * src.rgb - src.a) * (sqrt(dst_np) - dst_np),
                         step(dst.a, 4.0 * dst.rgb)),
                     step(src.a, 2.0 * src.rgb))
                 + src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a);
    result.a = src.a + dst.a - src.a * dst.a;
    return result;
}
