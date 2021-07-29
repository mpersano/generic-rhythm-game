#version 420 core

layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec3 normal;

uniform mat4 modelViewProjection;
uniform mat3 normalMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

out vec2 vs_texcoord;
out vec3 vs_position;
out vec3 vs_normal;

void main(void)
{
    vs_position = vec3(viewMatrix * modelMatrix * vec4(position, 1.0));
    vs_normal = normalMatrix * normal;
    vs_texcoord = texcoord;
    gl_Position = modelViewProjection * vec4(position, 1.0);
}
