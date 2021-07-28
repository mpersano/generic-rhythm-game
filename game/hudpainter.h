#pragma once

#include <gx/noncopyable.h>
#include <gx/shaderprogram.h>
#include <gx/util.h>

#include <memory>

namespace GX {
class FontCache;
class SpriteBatcher;
} // namespace GX

class HUDPainter : private GX::NonCopyable
{
public:
    explicit HUDPainter();
    ~HUDPainter();

    void resize(int width, int height);

    void startPainting();
    void donePainting();

    GX::BoxI textBoundingBox(const std::u32string &text);

    void drawText(float x, float y, const glm::vec4 &color, int depth, const std::u32string &text);

    struct Gradient {
        glm::vec2 from, to;
        glm::vec4 startColor, endColor;
    };
    void drawText(float x, float y, const Gradient &gradient, int depth, const std::u32string &text);

    void resetTransform();
    void scale(float sx, float sy);
    void scale(float s);
    void translate(float dx, float dy);
    void rotate(float angle);

private:
    void updateSceneBox(int width, int height);

    std::unique_ptr<GX::FontCache> m_fontCache;
    std::unique_ptr<GX::SpriteBatcher> m_spriteBatcher;
    std::unique_ptr<GX::GL::ShaderProgram> m_textProgram;
    GX::BoxF m_sceneBox = {};
    glm::mat4 m_transform; // TODO: glm::mat3
};
