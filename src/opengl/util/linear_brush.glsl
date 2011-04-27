uniform sampler1D palette;
uniform vec3 linear;
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

    float val = dot(linear.xy, A) * linear.z;

    return texture1D(palette, val);
}

