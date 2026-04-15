#include <Graphics/Texture/TextureLoader.h>

void TextureLoader::Cleanup() {
    // Clear all textures
    for (auto& pair : m_textures) pair.second.Destroy();
}

Texture* TextureLoader::LoadTexture(const std::string& path) {
    // Return texture if already loaded
    auto it = m_textures.find(path);
    if (it != m_textures.end()) return &it->second;

    // Load a new texture otherwise
    Texture texture;
    if (!texture.LoadFromFile(path)) return nullptr;

    m_textures[path] = std::move(texture);
    return &m_textures[path];
}