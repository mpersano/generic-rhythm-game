#version 420 core

uniform sampler2D spriteTexture;

in vec2 vs_texcoord;
in vec4 vs_fgColor;
in vec4 vs_bgColor;

out vec4 fragColor;

void main(void)
{
    vec4 t = texture(spriteTexture, vs_texcoord);
    vec4 color = mix(vs_bgColor, vs_fgColor, t.r);
    color.a *= t.a;
    fragColor = color;
}
