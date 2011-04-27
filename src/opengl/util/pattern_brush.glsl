uniform sampler2D brush_texture;
uniform vec2 inv_brush_texture_size;
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
    vec2 coords = hcoords.xy / hcoords.z;

    coords *= inv_brush_texture_size;

    float alpha = 1.0 - texture2D(brush_texture, coords).r;

    return gl_Color * alpha;
}
