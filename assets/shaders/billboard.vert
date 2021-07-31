#version 420 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 velocity;
layout(location=2) in vec2 size;
layout(location=3) in float alpha;

out WorldVertex {
    vec3 position;
    vec3 velocity;
    vec2 size;
    float alpha;
} vs_out;

void main(void)
{
    vs_out.position = position;
    vs_out.velocity = velocity;
    vs_out.size = size;
    vs_out.alpha = alpha;
}
