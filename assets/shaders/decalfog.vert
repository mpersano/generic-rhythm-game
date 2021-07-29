#version 420 core

layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;

uniform mat4 modelViewProjection;
uniform vec3 eye;

out vec2 vs_texcoord;
out float vs_distance;

void main(void)
{
    vs_texcoord = texcoord;
    vs_distance = distance(position, eye);
    gl_Position = modelViewProjection * vec4(position, 1.0);
}
