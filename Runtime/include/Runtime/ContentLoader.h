#pragma once
#include <Graphics/Texture.h>
#include <Graphics/Font.h>
#include <unordered_map>
#include <string>

/**
 * @brief Helper class to load textures
 */
class ContentLoader 
{
public:

    /**
     * @brief Loads or retrieves a texture by path
     * @param path File path to the texture
     * @return Pointer to Texture, nullptr if loading fails
     */
    Texture* LoadTexture(const std::string& path);

    /**
     * @brief Gets the file path of a loaded texture
     * @param texture Pointer to the texture
     * @return File path of the texture, or empty string if not found
     */
    std::string GetTexturePath(const Texture* texture);

    /**
     * @brief Loads or retrieves a font by path
     * @param path File path to the font
     * @param fontSize Size of the font
     * @return Pointer to Texture, nullptr if loading fails
     */
    Font* LoadFont(const std::string& path, float fontSize);

    /**
     * @brief Gets the file path of a loaded font
     * @param font Pointer to the font
     * @return File path of the font, or empty string if not found
     */
    std::string GetFontPath(const Font* font);
    
    /**
     * @brief Cleans up all loaded assets
     */
    void Cleanup();

private:
    // Map from paths to loaded textures
    std::unordered_map<std::string, Texture> m_textures;

    // Map from paths to loaded fonts
    std::unordered_map<std::string, Font> m_fonts;
};
