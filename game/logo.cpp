#include "logo.h"

#include "hudpainter.h"
#include "loadprogram.h"
#include "material.h"

#include <gx/spritebatcher.h>
#include <gx/texture.h>

using namespace std::string_literals;

Logo::Logo()
{
    m_program = loadProgram("logo.vert", nullptr, "logo.frag");
}

void Logo::draw(HUDPainter *hudPainter) const
{
    auto *spriteBatcher = hudPainter->spriteBatcher();

    spriteBatcher->setBatchProgram(m_program.get());

    const auto *texture = cachedTexture("logo.png"s);

    const auto left = -0.5f * texture->width();
    const auto right = 0.5f * texture->width();
    const auto top = 0.5f * texture->height();
    const auto bottom = -0.5f * texture->height();

    const glm::vec4 fgColor(1, 1, 1, 1);
    const glm::vec4 bgColor(1, .64, 0, 1);

    static const GX::SpriteBatcher::QuadVerts verts = {
        { { { left, top }, { 0, 0 }, fgColor, bgColor },
          { { right, top }, { 1, 0 }, fgColor, bgColor },
          { { right, bottom }, { 1, 1 }, fgColor, bgColor },
          { { left, bottom }, { 0, 1 }, fgColor, bgColor } }
    };
    spriteBatcher->addSprite(cachedTexture("logo.png"s), verts, 0);
}
