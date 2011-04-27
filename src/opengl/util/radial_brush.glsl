uniform sampler1D palette;
uniform vec2 fmp;
uniform float fmp2_m_radius2;
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
    vec2 B = fmp;

    float a = fmp2_m_radius2;
    float b = 2.0*dot(A, B);
    float c = -dot(A, A);

    float val = (-b + sqrt(b*b - 4.0*a*c)) / (2.0*a);

    return texture1D(palette, val);
}

