#version 420 core

layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec4 fgColor;
layout(location=3) in vec4 bgColor;

uniform mat4 mvp;

out vec2 vs_texcoord;
out vec4 vs_fgColor;
out vec4 vs_bgColor;

void main(void)
{
    vs_texcoord = texcoord;
    vs_fgColor = fgColor;
    vs_bgColor = bgColor;
    gl_Position = mvp * vec4(position, 0, 1);
}
