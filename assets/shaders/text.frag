#version 420 core

uniform sampler2D spriteTexture;

in vec2 vs_texcoord;
in vec4 vs_color;

out vec4 fragColor;

void main(void)
{
    float alpha = texture(spriteTexture, vs_texcoord).r;
    vec4 color = vs_color;
    color.a *= alpha;
    fragColor = color;
}
