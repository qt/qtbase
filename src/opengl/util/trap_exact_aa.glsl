float quad_aa()
{
    float top = min(gl_FragCoord.y + 0.5, gl_TexCoord[0].x);
    float bottom = max(gl_FragCoord.y - 0.5, gl_TexCoord[0].y);

    float area = top - bottom;

    float left = gl_FragCoord.x - 0.5;
    float right = gl_FragCoord.x + 0.5;

    // use line equations to compute intersections of left/right edges with top/bottom of truncated pixel
    vec4 vecX = gl_TexCoord[1].xxzz * vec2(top, bottom).xyxy + gl_TexCoord[1].yyww;

    vec2 invA = gl_TexCoord[0].zw;

    // transform right line to left to be able to use same calculations for both
    vecX.zw = 2.0 * gl_FragCoord.x - vecX.zw;

    vec2 topX = vec2(vecX.x, vecX.z);
    vec2 bottomX = vec2(vecX.y, vecX.w);

    // transform lines such that top intersection is to the right of bottom intersection
    vec2 topXTemp = max(topX, bottomX);
    vec2 bottomXTemp = min(topX, bottomX);

    // make sure line slope reflects mirrored lines
    invA = mix(invA, -invA, step(topX, bottomX));

    vec2 vecLeftRight = vec2(left, right);

    // compute the intersections of the lines with the left and right edges of the pixel
    vec4 intersectY = bottom + (vecLeftRight.xyxy - bottomXTemp.xxyy) * invA.xxyy;

    vec2 temp = mix(area - 0.5 * (right - bottomXTemp) * (intersectY.yw - bottom), // left < bottom < right < top
                    (0.5 * (topXTemp + bottomXTemp) - left) * area,    // left < bottom < top < right
                    step(topXTemp, right.xx));

    vec2 excluded = 0.5 * (top - intersectY.xz) * (topXTemp - left); // bottom < left < top < right

    excluded = mix((top - 0.5 * (intersectY.yw + intersectY.xz)) * (right - left), // bottom < left < right < top
                   excluded, step(topXTemp, right.xx));

    excluded = mix(temp, // left < bottom < right (see calculation of temp)
                   excluded, step(bottomXTemp, left.xx));

    excluded = mix(vec2(area, area), // right < bottom < top
                   excluded, step(bottomXTemp, right.xx));

    excluded *= step(left, topXTemp);

    return (area - excluded.x - excluded.y) * step(bottom, top);
}

void main()
{
    gl_FragColor = quad_aa().xxxx;
}

