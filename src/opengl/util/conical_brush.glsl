// conical gradient shader
#define M_PI  3.14159265358979323846
uniform sampler1D palette;
uniform float angle;
uniform vec3 inv_matrix_m0;
uniform vec3 inv_matrix_m1;
uniform vec3 inv_matrix_m2;

vec4 brush()
{
    mat3 mat;

    mat[0] = inv_matrix_m0;
    mat[1] = inv_matrix_m1;
    mat[2] = inv_matrix_m2;

    vec3 hcoords = mat * vec3(gl_FragCoord.xy, 1);
    vec2 A = hcoords.xy / hcoords.z;

/*     float val = fmod((atan2(-A.y, A.x) + angle) / (2.0 * M_PI), 1); */
    if (abs(A.y) == abs(A.x))
 	A.y += 0.002;
    float t = (atan(-A.y, A.x) + angle) / (2.0 * M_PI);
    float val = t - floor(t);
    return texture1D(palette, val);
}

