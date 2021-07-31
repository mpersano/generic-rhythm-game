#pragma once

#include <gx/noncopyable.h>

#include <memory>

class HUDPainter;

namespace GX::GL {
class ShaderProgram;
};

class Logo : private GX::NonCopyable
{
public:
    Logo();

    void draw(HUDPainter *hudPainter) const;

private:
    std::unique_ptr<GX::GL::ShaderProgram> m_program;
};
