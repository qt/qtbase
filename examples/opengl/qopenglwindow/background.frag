uniform int currentTime;
uniform highp vec2 windowSize;

highp float noise(highp vec2 co)
{
    return 0.5 * fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

highp float curvSpeed()
{
   return mod(float(currentTime), 1000000.0) / 500.0;
}

highp float curv()
{
    return 1.0 - abs((gl_FragCoord.y / (windowSize.y / 10.0) - 5.0) - sin((gl_FragCoord.x / (windowSize.x/20.0)) - curvSpeed()));
}

void main()
{
    highp float coordNoise = noise(gl_FragCoord.xy);
    highp float proximity = smoothstep(0.5, 1.0, (curv() + 1.0) * (coordNoise ));
    highp vec3 color = vec3(coordNoise) * proximity;
    gl_FragColor = vec4(color, 1.0);
}
