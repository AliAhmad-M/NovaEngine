#include <Graphics/Texture/TextureLoader.h>
#include <glad/glad.h>

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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    m_textures[path] = std::move(texture);
    return &m_textures[path];
}