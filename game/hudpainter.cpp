#include "hudpainter.h"

#include "loadprogram.h"

#include <gx/fontcache.h>
#include <gx/spritebatcher.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
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

void HUDPainter::drawText(const glm::vec2 &position, const glm::vec4 &color, int depth, const std::u32string &text)
{
    const auto boundingBox = textBoundingBox(text);

    const auto xOffset = -boundingBox.min.x - 0.5f * (boundingBox.max.x - boundingBox.min.x);
    auto glyphPosition = position + glm::vec2(xOffset, 0);

    m_spriteBatcher->setBatchProgram(m_textProgram.get());

    for (char32_t ch : text) {
        const auto glyph = m_fontCache->getGlyph(ch);
        if (!glyph)
            continue;
        const auto p0 = glyphPosition + glm::vec2(glyph->boundingBox.min);
        const auto p1 = p0 + glm::vec2(glyph->boundingBox.max - glyph->boundingBox.min);
        m_spriteBatcher->addSprite(glyph->pixmap, p0, p1, color, depth);
        glyphPosition += glm::vec2(glyph->advanceWidth, 0);
    }
}

void HUDPainter::drawGradientText(const glm::vec2 &position, const Gradient &gradient, int depth, const std::u32string &text)
{
    const auto boundingBox = textBoundingBox(text);

    const auto xOffset = -boundingBox.min.x - 0.5f * (boundingBox.max.x - boundingBox.min.x);
    const auto startPosition = position + glm::vec2(xOffset, 0);

    const auto vertexColor = [&boundingBox, &gradient, startPosition](const glm::vec2 &p) {
        const glm::vec2 v {
            (p.x - startPosition.x - boundingBox.min.x) / (boundingBox.max.x - boundingBox.min.x),
            (p.y - startPosition.y - boundingBox.min.y) / (boundingBox.max.y - boundingBox.min.y)
        };
        const auto t = glm::dot(v - gradient.from, gradient.to - gradient.from);
        return glm::mix(gradient.startColor, gradient.endColor, glm::clamp(t, 0.0f, 1.0f));
    };

    m_spriteBatcher->setBatchProgram(m_textProgram.get());

    auto glyphPosition = startPosition;

    for (char32_t ch : text) {
        const auto glyph = m_fontCache->getGlyph(ch);
        if (!glyph)
            continue;
        const auto p0 = glyphPosition + glm::vec2(glyph->boundingBox.min);
        const auto p1 = p0 + glm::vec2(glyph->boundingBox.max - glyph->boundingBox.min);

        const auto &pixmap = glyph->pixmap;

        const auto &textureCoords = pixmap.textureCoords;
        const auto &t0 = textureCoords.min;
        const auto &t1 = textureCoords.max;

        const GX::SpriteBatcher::QuadVerts verts = {
            { { { p0.x, p0.y }, { t0.x, t0.y }, vertexColor({ p0.x, p0.y }), glm::vec4(0) },
              { { p1.x, p0.y }, { t1.x, t0.y }, vertexColor({ p1.x, p0.y }), glm::vec4(0) },
              { { p1.x, p1.y }, { t1.x, t1.y }, vertexColor({ p1.x, p1.y }), glm::vec4(0) },
              { { p0.x, p1.y }, { t0.x, t1.y }, vertexColor({ p0.x, p1.y }), glm::vec4(0) } }
        };

        m_spriteBatcher->addSprite(pixmap.texture, verts, depth);
        glyphPosition += glm::vec2(glyph->advanceWidth, 0);
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

        offset += glm::ivec2(glyph->advanceWidth, 0);
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
