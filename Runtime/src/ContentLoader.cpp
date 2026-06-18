#include <Runtime/ContentLoader.h>
#include <glad/glad.h>

Texture* ContentLoader::LoadTexture(const std::string& path) 
{
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

std::string ContentLoader::GetTexturePath(const Texture* texture)
{
	static std::string empty;
    if (!texture) 
        return empty;

	for (const auto& pair : m_textures)
		if (&pair.second == texture) return pair.first;
	
    return empty;
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

std::string ContentLoader::GetFontPath(const Font* font)
{
	static std::string empty;
	
    if (!font)
		return empty;

	for (const auto& pair : m_fonts)
		if (&pair.second == font) return pair.first;

	return empty;
}

void ContentLoader::Cleanup()
{
    // Clear all textures
    for (auto& pair : m_textures) pair.second.Destroy();

    // Clear all fonts
    for (auto& pair : m_fonts) pair.second.Destroy();
}