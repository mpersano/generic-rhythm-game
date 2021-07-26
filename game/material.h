#pragma once

#include "shadermanager.h"

#include <memory>
#include <string>

namespace GX::GL {
class Texture;
}

struct Material {
    ShaderManager::Program program;
    enum Flags {
        None = 0,
        Transparent = 1,
    };
    unsigned flags;
    const GX::GL::Texture *texture;
};

GX::GL::Texture *cachedTexture(const std::string &textureName);
