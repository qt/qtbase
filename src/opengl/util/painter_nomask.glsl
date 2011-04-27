uniform sampler2D dst_texture;
uniform vec2 inv_dst_size;

void main()
{
    vec4 dst = texture2D(dst_texture, gl_FragCoord.xy * inv_dst_size);

    gl_FragColor = composite(brush(), dst);
}
