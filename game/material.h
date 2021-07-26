#pragma once

#include "shadermanager.h"

#include <memory>
#include <string>

namespace GX::GL {
class Texture;
}

struct Material {
    ShaderManager::Program program;
    const GX::GL::Texture *texture;
};

GX::GL::Texture *cachedTexture(const std::string &textureName);
