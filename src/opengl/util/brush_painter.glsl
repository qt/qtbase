// fast brush painter for composition modes which can be implemented with blendfuncs
// no mask, used for fast filling of aliased primitives (or multisampled)

void main()
{
    gl_FragColor = brush();
}
