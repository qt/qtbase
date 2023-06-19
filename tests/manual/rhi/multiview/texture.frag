#version 440

layout(location = 0) in vec2 v_texcoord;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2DArray tex;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    int flip;
    float layer;
};

void main()
{
    vec4 c = texture(tex, vec3(v_texcoord, layer));
    fragColor = vec4(c.rgb * c.a, c.a);
}
