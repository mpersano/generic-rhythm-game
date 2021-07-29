#version 420 core

uniform sampler2D baseColorTexture;
uniform vec2 fogDistance; // near, far
uniform vec4 fogColor;

in vec2 vs_texcoord;
in float vs_distance;

out vec4 fragColor;

void main(void)
{
    float fogFactor = clamp((fogDistance.y - vs_distance) / (fogDistance.y - fogDistance.x), 0.0, 1.0);
    vec4 baseColor = texture(baseColorTexture, vs_texcoord);
    fragColor = mix(fogColor, baseColor, fogFactor);
}
