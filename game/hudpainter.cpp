#include "hudpainter.h"

#include "loadprogram.h"

#include <gx/fontcache.h>
#include <gx/spritebatcher.h>

#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

HUDPainter::HUDPainter()
    : m_fontCache(new GX::FontCache)
    , m_spriteBatcher(new GX::SpriteBatcher)
{
    const auto fontPath = "assets/fonts/OpenSans_Regular.ttf"s;
    if (!m_fontCache->load(fontPath, 120)) {
        spdlog::error("Failed to load font {}", fontPath);
    }

    m_textProgram = loadProgram("text.vert", nullptr, "text.frag");
}

HUDPainter::~HUDPainter() = default;

void HUDPainter::resize(int width, int height)
{
    updateSceneBox(width, height);

    const auto projectionMatrix = glm::ortho(m_sceneBox.min.x, m_sceneBox.max.x, m_sceneBox.max.y, m_sceneBox.min.y, -1.0f, 1.0f);
    m_spriteBatcher->setTransformMatrix(projectionMatrix);
}

void HUDPainter::startPainting()
{
    m_spriteBatcher->startBatch();
}

void HUDPainter::donePainting()
{
    m_spriteBatcher->renderBatch();
}

void HUDPainter::drawText(const glm::vec2 &pos, const std::u32string &text)
{
    const auto boundingBox = textBoundingBox(text);

    const auto xOffset = -boundingBox.min.x - 0.5f * (boundingBox.max.x - boundingBox.min.x);
    auto position = pos + glm::vec2(xOffset, 14);

    m_spriteBatcher->setBatchProgram(m_textProgram.get());

    for (char32_t ch : text) {
        const auto glyph = m_fontCache->getGlyph(ch);
        if (!glyph)
            continue;
        const auto p0 = position + glm::vec2(glyph->boundingBox.min);
        const auto p1 = p0 + glm::vec2(glyph->boundingBox.max - glyph->boundingBox.min);
        m_spriteBatcher->addSprite(glyph->pixmap, p0, p1, glm::vec4(1, .85, 0, 1), 0);
        position += glm::vec2(glyph->advanceWidth, 0);
    }
}

GX::BoxI HUDPainter::textBoundingBox(const std::u32string &text)
{
    GX::BoxI result;
    auto offset = glm::ivec2(0);

    for (char32_t ch : text) {
        const auto glyph = m_fontCache->getGlyph(ch);
        if (!glyph) {
            spdlog::warn("Failed to locate glyph {}", static_cast<int>(ch));
            continue;
        }

        GX::BoxI glyphBoundingBox = glyph->boundingBox;
        glyphBoundingBox += offset;
        result |= glyphBoundingBox;

        offset += glm::ivec2(glyph->advanceWidth);
    }

    return result;
}

void HUDPainter::updateSceneBox(int width, int height)
{
    static constexpr auto PreferredSceneSize = glm::vec2(1200, 600);
    static constexpr auto PreferredAspectRatio = PreferredSceneSize.x / PreferredSceneSize.y;

    const auto aspectRatio = static_cast<float>(width) / height;
    const auto sceneSize = [aspectRatio]() -> glm::vec2 {
        if (aspectRatio > PreferredAspectRatio) {
            const auto height = PreferredSceneSize.y;
            return { height * aspectRatio, height };
        } else {
            const auto width = PreferredSceneSize.x;
            return { width, width / aspectRatio };
        }
    }();

    const auto left = -0.5f * sceneSize.x;
    const auto right = 0.5f * sceneSize.x;
    const auto top = 0.5f * sceneSize.y;
    const auto bottom = -0.5f * sceneSize.y;
    m_sceneBox = GX::BoxF { { left, bottom }, { right, top } };
}
