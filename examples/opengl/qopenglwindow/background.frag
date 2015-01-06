#define M_PI 3.1415926535897932384626433832795
#define SPEED 10000.0

uniform int currentTime;
uniform highp vec2 windowSize;

highp float noise(highp vec2 co)
{
    return 0.5 * fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

highp float curvSpeed()
{
   return (mod(float(currentTime), SPEED) / SPEED) * (2.0 * M_PI);
}

highp float curv(int curvCount)
{
    highp float curv_y = 0.1 *(cos((gl_FragCoord.x / windowSize.x) * (float(curvCount * 2) * M_PI) - curvSpeed())) + 0.5;
    highp float frag_y = gl_FragCoord.y / windowSize.y;
    return 1.0 - abs(curv_y - frag_y);
}

void main()
{
    highp float coordNoise = noise(gl_FragCoord.xy);
    highp float proximity = smoothstep(0.85, 1.0, (curv(6) + 1.0) * (coordNoise ));
    highp vec3 color = vec3(coordNoise) * proximity;
    gl_FragColor = vec4(color, 1.0);
}
