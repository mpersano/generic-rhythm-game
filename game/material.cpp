#include "material.h"

#include <gx/pixmap.h>
#include <gx/texture.h>

#include <unordered_map>

namespace {

std::string texturePath(const std::string &basename)
{
    return std::string("assets/textures/") + basename;
}

} // namespace

GX::GL::Texture *cachedTexture(const std::string &textureName)
{
    if (textureName.empty())
        return nullptr;
    static std::unordered_map<std::string, std::unique_ptr<GX::GL::Texture>> cache;
    auto it = cache.find(textureName);
    if (it == cache.end()) {
        auto texture = std::make_unique<GX::GL::Texture>(GX::loadPixmap(texturePath(textureName)));
        it = cache.emplace(textureName, std::move(texture)).first;
    }
    return it->second.get();
}
