// Dca' = Sca + Dca - Sca.Dca
// Da'  = Sa + Da - Sa.Da 
vec4 composite(vec4 src, vec4 dst)
{
    return src + dst - src * dst;
}
