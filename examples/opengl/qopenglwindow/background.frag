uniform highp int currentTime;
uniform highp vec2 windowSize;

float noise(vec2 co)
{
    return 0.5 * fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

float curvSpeed()
{
   return mod(float(currentTime), 1000000.0) / 500.0;
}

float curv()
{
    return 1.0 - abs((gl_FragCoord.y / (windowSize.y / 10.0) - 5.0) - sin((gl_FragCoord.x / (windowSize.x/20.0)) - curvSpeed()));
}

void main()
{
    float coordNoise = noise(gl_FragCoord.xy);
    float proximity = smoothstep(0.5, 1.0, (curv() + 1.0) * (coordNoise ));
    gl_FragColor = vec4(coordNoise, coordNoise, coordNoise, 1.0) * proximity;
}
