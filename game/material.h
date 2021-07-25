#pragma once

#include "shadermanager.h"

#include <memory>
#include <string>

namespace GX::GL {
class Texture;
}

struct MaterialKey {
    ShaderManager::Program program;
    std::string textureName;
    bool operator==(const MaterialKey &other) const
    {
        return program == other.program && textureName == other.textureName;
    }
};

struct Material {
    ShaderManager::Program program = ShaderManager::Debug;
    const GX::GL::Texture *texture = nullptr;
};

GX::GL::Texture *cachedTexture(const std::string &textureName);
Material *cachedMaterial(const MaterialKey &key);
