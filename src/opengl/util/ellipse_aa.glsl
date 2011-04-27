uniform vec3 inv_matrix_m0;
uniform vec3 inv_matrix_m1;
uniform vec3 inv_matrix_m2;

uniform vec2 ellipse_offset;

// ellipse equation

// s^2/a^2 + t^2/b^2 = 1
//
// implicit equation:
// g(s,t) = 1 - s^2/r_s^2 - t^2/r_t^2

// distance from ellipse:
// grad = [dg/dx dg/dy]
// d(s, t) ~= g(s, t) / |grad|

// dg/dx = dg/ds * ds/dx + dg/dt * dt/dx
// dg/dy = dg/ds * ds/dy + dg/dt * dt/dy

float ellipse_aa()
{
    mat3 mat;

    mat[0] = inv_matrix_m0;
    mat[1] = inv_matrix_m1;
    mat[2] = inv_matrix_m2;

    vec3 hcoords = mat * vec3(gl_FragCoord.xy + ellipse_offset, 1);
    float inv_w = 1.0 / hcoords.z;
    vec2 st = hcoords.xy * inv_w;

    vec4 xy = vec4(mat[0].xy, mat[1].xy);
    vec2 h = vec2(mat[0].z, mat[1].z);

    vec4 dstdxy = (xy.xzyw - h.xyxy * st.xxyy) * inv_w;

    //dstdxy.x = (mat[0].x - mat[0].z * st.x) * inv_w; // ds/dx
    //dstdxy.y = (mat[1].x - mat[1].z * st.x) * inv_w; // ds/dy
    //dstdxy.z = (mat[0].y - mat[0].z * st.y) * inv_w; // dt/dx
    //dstdxy.w = (mat[1].y - mat[1].z * st.y) * inv_w; // dt/dy

    vec2 inv_r = gl_TexCoord[0].xy;
    vec2 n = st * inv_r;
    float g = 1.0 - dot(n, n);

    vec2 dgdst = -2.0 * n * inv_r;

    vec2 grad = vec2(dot(dgdst, dstdxy.xz),
                     dot(dgdst, dstdxy.yw));

    return smoothstep(-0.5, 0.5, g * inversesqrt(dot(grad, grad)));
}

void main()
{
    gl_FragColor = ellipse_aa().xxxx;
}
