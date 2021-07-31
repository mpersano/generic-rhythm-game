#version 420 core

uniform sampler2D baseColorTexture;
uniform vec3 lightPosition;
uniform vec3 eye;
uniform vec2 fogDistance; // near, far
uniform vec4 fogColor;
uniform vec4 clipPlane;

in vec2 vs_texcoord;
in vec3 vs_position;
in vec3 vs_worldPosition;
in vec3 vs_normal;
in float vs_distance;

out vec4 fragColor;

const vec3 ka = vec3(.1);
const vec3 ks = vec3(.8);
const float shininess = 50.0;

vec3 ads(vec3 baseColor, vec3 lightPosition, float lightIntensity)
{
    vec3 n = normalize(vs_normal);
    vec3 s = normalize(lightPosition - vs_position);
    vec3 v = normalize(-vs_position);
    vec3 h = normalize(v + s);
    return lightIntensity * (ka + baseColor * max(dot(s, n), 0.0) + ks * pow(max(dot(h, n), 0.0), shininess));
}

void main(void)
{
    if (dot(vec4(vs_worldPosition, 1), clipPlane) < 0)
        discard;
    vec4 baseColor = texture(baseColorTexture, vs_texcoord);
    vec3 color = ads(baseColor.xyz, lightPosition, 1.0) + ads(baseColor.xyz, eye, 0.25);
    float fogFactor = clamp((fogDistance.y - vs_distance) / (fogDistance.y - fogDistance.x), 0.0, 1.0);
    fragColor = mix(fogColor, vec4(color, baseColor.a), fogFactor);
}
