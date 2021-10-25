uniform sampler2D texture;
varying vec2 v_texcoord;

void main()
{
    gl_FragColor = texture2D(texture, v_texcoord);
}

