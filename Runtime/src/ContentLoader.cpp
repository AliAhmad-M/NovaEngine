#include <Runtime/ContentLoader.h>
#include <glad/glad.h>

void ContentLoader::Cleanup() {
    // Clear all textures
    for (auto& pair : m_textures) pair.second.Destroy();

    // Clear all fonts
    for (auto& pair : m_fonts) pair.second.Destroy();
}

Texture* ContentLoader::LoadTexture(const std::string& path) {
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

Font* ContentLoader::LoadFont(const std::string& path, float fontSize) 
{
    // Return font if already loaded
    auto it = m_fonts.find(path);
    if (it != m_fonts.end()) return &it->second;

    // Load a new font otherwise
    Font font;
    if (!font.LoadFromFile(path, fontSize)) return nullptr;
    m_fonts[path] = std::move(font); return &m_fonts[path];
}