// fast painter for composition modes which can be implemented with blendfuncs

uniform sampler2D mask_texture;
uniform vec2 inv_mask_size;
uniform vec2 mask_offset;
uniform vec4 mask_channel;

float mask()
{
    return dot(mask_channel, texture2D(mask_texture, (gl_FragCoord.xy + mask_offset) * inv_mask_size));
}

void main()
{
    // combine clip and coverage channels
    float mask_alpha = mask();

    gl_FragColor = brush() * mask_alpha;
}
