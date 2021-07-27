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

    void drawText(const glm::vec2 &pos, const std::u32string &text);
    GX::BoxI textBoundingBox(const std::u32string &text);

private:
    void updateSceneBox(int width, int height);

    std::unique_ptr<GX::FontCache> m_fontCache;
    std::unique_ptr<GX::SpriteBatcher> m_spriteBatcher;
    std::unique_ptr<GX::GL::ShaderProgram> m_textProgram;
    GX::BoxF m_sceneBox = {};
};
