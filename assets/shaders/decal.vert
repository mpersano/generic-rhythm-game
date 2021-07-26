#version 420 core

layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;

uniform mat4 modelViewProjection;

out vec2 vs_texcoord;

void main(void)
{
    vs_texcoord = texcoord;
    gl_Position = modelViewProjection * vec4(position, 1.0);
}
