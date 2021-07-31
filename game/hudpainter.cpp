#include "hudpainter.h"

#include "loadprogram.h"

#include <gx/fontcache.h>
#include <gx/spritebatcher.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

namespace {

static const auto fontPath = "assets/fonts/OpenSans_Regular.ttf"s;

}

HUDPainter::HUDPainter()
    : m_spriteBatcher(new GX::SpriteBatcher)
{
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
    m_transformStack.clear();
    resetTransform();
    m_font = nullptr;
    m_spriteBatcher->startBatch();
}

void HUDPainter::donePainting()
{
    m_spriteBatcher->renderBatch();
}

void HUDPainter::setFont(const Font &font)
{
    auto it = m_fonts.find(font);
    if (it == m_fonts.end()) {
        auto fontCache = std::make_unique<GX::FontCache>();
        if (!fontCache->load(font.fontPath, font.pixelHeight)) {
            spdlog::error("Failed to load font {}", font.fontPath);
        }
        it = m_fonts.emplace(font, std::move(fontCache)).first;
    }
    m_font = it->second.get();
}

void HUDPainter::drawText(float x, float y, const glm::vec4 &color, int depth, const std::u32string &text, Alignment alignment)
{
    if (!m_font)
        return;

    const auto boundingBox = textBoundingBox(text);
    // ugh repeated code
    const auto xOffset = [&boundingBox, alignment]() -> float {
        switch (alignment) {
            case Alignment::Left:
                return -boundingBox.min.x;
            case Alignment::Center:
            default:
                return -boundingBox.min.x - 0.5f * (boundingBox.max.x - boundingBox.min.x);
            case Alignment::Right:
                return -0.5f * (boundingBox.max.x - boundingBox.min.x); // XXX check this?
        }
    }();

    auto glyphPosition = glm::vec2(x, y) + glm::vec2(xOffset, 0);

    m_spriteBatcher->setBatchProgram(m_textProgram.get());

    for (char32_t ch : text) {
        const auto glyph = m_font->getGlyph(ch);
        if (!glyph)
            continue;
        const auto p0 = glyphPosition + glm::vec2(glyph->boundingBox.min);
        const auto p1 = p0 + glm::vec2(glyph->boundingBox.max - glyph->boundingBox.min);

        const auto &pixmap = glyph->pixmap;

        const auto &textureCoords = pixmap.textureCoords;
        const auto &t0 = textureCoords.min;
        const auto &t1 = textureCoords.max;

        const auto v0 = glm::vec2(m_transform * glm::vec4(p0.x, p0.y, 0, 1));
        const auto v1 = glm::vec2(m_transform * glm::vec4(p1.x, p0.y, 0, 1));
        const auto v2 = glm::vec2(m_transform * glm::vec4(p1.x, p1.y, 0, 1));
        const auto v3 = glm::vec2(m_transform * glm::vec4(p0.x, p1.y, 0, 1));

        const GX::SpriteBatcher::QuadVerts verts = {
            { { v0, { t0.x, t0.y }, color, glm::vec4(0) },
              { v1, { t1.x, t0.y }, color, glm::vec4(0) },
              { v2, { t1.x, t1.y }, color, glm::vec4(0) },
              { v3, { t0.x, t1.y }, color, glm::vec4(0) } }
        };

        m_spriteBatcher->addSprite(pixmap.texture, verts, depth);

        glyphPosition += glm::vec2(glyph->advanceWidth, 0);
    }
}

void HUDPainter::drawText(float x, float y, const Gradient &gradient, int depth, const std::u32string &text, Alignment alignment)
{
    if (!m_font)
        return;

    const auto boundingBox = textBoundingBox(text);
    const auto xOffset = [&boundingBox, alignment]() -> float {
        switch (alignment) {
            case Alignment::Left:
                return -boundingBox.min.x;
            case Alignment::Center:
            default:
                return -boundingBox.min.x - 0.5f * (boundingBox.max.x - boundingBox.min.x);
            case Alignment::Right:
                return -0.5f * (boundingBox.max.x - boundingBox.min.x); // XXX check this?
        }
    }();

    const auto startPosition = glm::vec2(x, y) + glm::vec2(xOffset, 0);

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
        const auto glyph = m_font->getGlyph(ch);
        if (!glyph)
            continue;
        const auto p0 = glyphPosition + glm::vec2(glyph->boundingBox.min);
        const auto p1 = p0 + glm::vec2(glyph->boundingBox.max - glyph->boundingBox.min);

        const auto &pixmap = glyph->pixmap;

        const auto &textureCoords = pixmap.textureCoords;
        const auto &t0 = textureCoords.min;
        const auto &t1 = textureCoords.max;

        const auto v0 = glm::vec2(m_transform * glm::vec4(p0.x, p0.y, 0, 1));
        const auto v1 = glm::vec2(m_transform * glm::vec4(p1.x, p0.y, 0, 1));
        const auto v2 = glm::vec2(m_transform * glm::vec4(p1.x, p1.y, 0, 1));
        const auto v3 = glm::vec2(m_transform * glm::vec4(p0.x, p1.y, 0, 1));

        const GX::SpriteBatcher::QuadVerts verts = {
            { { v0, { t0.x, t0.y }, vertexColor({ p0.x, p0.y }), glm::vec4(0) },
              { v1, { t1.x, t0.y }, vertexColor({ p1.x, p0.y }), glm::vec4(0) },
              { v2, { t1.x, t1.y }, vertexColor({ p1.x, p1.y }), glm::vec4(0) },
              { v3, { t0.x, t1.y }, vertexColor({ p0.x, p1.y }), glm::vec4(0) } }
        };

        m_spriteBatcher->addSprite(pixmap.texture, verts, depth);
        glyphPosition += glm::vec2(glyph->advanceWidth, 0);
    }
}

GX::BoxI HUDPainter::textBoundingBox(const std::u32string &text)
{
    if (!m_font)
        return {};

    GX::BoxI result;
    auto offset = glm::ivec2(0);

    for (char32_t ch : text) {
        const auto glyph = m_font->getGlyph(ch);
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

void HUDPainter::resetTransform()
{
    m_transform = glm::mat4(1);
}

void HUDPainter::scale(const glm::vec2 &s)
{
    m_transform = glm::scale(m_transform, glm::vec3(s, 1));
}

void HUDPainter::scale(float sx, float sy)
{
    scale(glm::vec2(sx, sy));
}

void HUDPainter::scale(float s)
{
    scale(s, s);
}

void HUDPainter::translate(const glm::vec2 &p)
{
    m_transform = glm::translate(m_transform, glm::vec3(p, 0));
}

void HUDPainter::translate(float dx, float dy)
{
    translate(glm::vec2(dx, dy));
}

void HUDPainter::rotate(float angle)
{
    m_transform = glm::rotate(m_transform, angle, glm::vec3(0, 0, 1));
}

void HUDPainter::saveTransform()
{
    m_transformStack.push_back(m_transform);
}

void HUDPainter::restoreTransform()
{
    if (m_transformStack.empty()) {
        spdlog::warn("Transform stack underflow lol");
        return;
    }
    m_transform = m_transformStack.back();
    m_transformStack.pop_back();
}

std::size_t HUDPainter::FontHasher::operator()(const Font &font) const
{
    std::size_t hash = 17;
    hash = hash * 31 + static_cast<std::size_t>(font.pixelHeight);
    hash = hash * 31 + std::hash<std::string>()(font.fontPath);
    return hash;
}
