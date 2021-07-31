#version 420 core

uniform sampler2D baseColorTexture;

in SpriteVertex {
    vec2 texCoord;
    float alpha;
} fs_in;

out vec4 fragColor;

void main(void)
{
    fragColor = fs_in.alpha * texture(baseColorTexture, fs_in.texCoord);
}
