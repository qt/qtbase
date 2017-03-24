#version 440

layout(location = 0) in vec3 vECVertNormal;
layout(location = 1) in vec3 vECVertPos;
layout(location = 2) flat in vec3 vDiffuseAdjust;

layout(std140, binding = 1) uniform buf {
    vec3 ECCameraPosition;
    vec3 ka;
    vec3 kd;
    vec3 ks;
    // Have one light only for now.
    vec3 ECLightPosition;
    vec3 attenuation;
    vec3 color;
    float intensity;
    float specularExp;
} ubuf;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 unnormL = ubuf.ECLightPosition - vECVertPos;
    float dist = length(unnormL);
    float att = 1.0 / (ubuf.attenuation.x + ubuf.attenuation.y * dist + ubuf.attenuation.z * dist * dist);

    vec3 N = normalize(vECVertNormal);
    vec3 L = normalize(unnormL);
    float NL = max(0.0, dot(N, L));
    vec3 dColor = att * ubuf.intensity * ubuf.color * NL;

    vec3 R = reflect(-L, N);
    vec3 V = normalize(ubuf.ECCameraPosition - vECVertPos);
    float RV = max(0.0, dot(R, V));
    vec3 sColor = att * ubuf.intensity * ubuf.color * pow(RV, ubuf.specularExp);

    fragColor = vec4(ubuf.ka + (ubuf.kd + vDiffuseAdjust) * dColor + ubuf.ks * sColor, 1.0);
}
