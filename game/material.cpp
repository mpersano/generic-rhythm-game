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
    if (it != cache.end()) {
        auto texture = std::make_unique<GX::GL::Texture>(GX::loadPixmap(texturePath(textureName)));
        it = cache.emplace(textureName, std::move(texture)).first;
    }
    return it->second.get();
}

Material *cachedMaterial(const MaterialKey &key)
{
    struct KeyHasher {
        std::size_t operator()(const MaterialKey &key) const
        {
            std::size_t hash = 17;
            hash = hash * 31 + static_cast<std::size_t>(key.program);
            hash = hash * 31 + std::hash<std::string>()(key.textureName);
            return hash;
        }
    };
    static std::unordered_map<MaterialKey, std::unique_ptr<Material>, KeyHasher> cache;
    auto it = cache.find(key);
    if (it == cache.end()) {
        auto material = std::unique_ptr<Material>(new Material { key.program, cachedTexture(key.textureName) });
        it = cache.emplace(key, std::move(material)).first;
    }
    return it->second.get();
}
