uniform vec2 porterduff_ab;
uniform vec3 porterduff_xyz;

vec4 composite(vec4 src, vec4 dst)
{
    vec4 result;

    result.xyz = porterduff_ab.x * src.xyz * dst.a
               + porterduff_ab.y * dst.xyz * src.a
               + porterduff_xyz.y * src.xyz * (1.0 - dst.a)
               + porterduff_xyz.z * dst.xyz * (1.0 - src.a);

    result.a = dot(porterduff_xyz, vec3(src.a * dst.a, src.a * (1.0 - dst.a), dst.a * (1.0 - src.a)));

    return result;
}
