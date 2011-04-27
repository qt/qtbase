uniform sampler2D dst_texture;
uniform sampler2D mask_texture;
uniform vec2 inv_mask_size;
uniform vec2 inv_dst_size;
uniform vec2 mask_offset;
uniform vec4 mask_channel;

float mask()
{
    return dot(mask_channel, texture2D(mask_texture, (gl_FragCoord.xy + mask_offset) * inv_mask_size));
}

void main()
{
    vec4 dst = texture2D(dst_texture, gl_FragCoord.xy * inv_dst_size);

    // combine clip and coverage channels
    float mask_alpha = mask();

    gl_FragColor = mix(dst, composite(brush(), dst), mask_alpha);
}
