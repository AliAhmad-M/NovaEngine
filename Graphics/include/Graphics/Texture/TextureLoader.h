#pragma once
#include "Texture.h"
#include <unordered_map>
#include <string>

/**
 * @brief Helper class to load textures
 */
class TextureLoader {
public:

    /**
     * @brief Loads or retrieves a texture by path.
     * @param path File path to the texture.
     * @return Pointer to Texture, nullptr if loading fails.
     */
    Texture* LoadTexture(const std::string& path);
    
    /**
     * @brief Cleans up all loaded textures
     */
    void Cleanup();

private:
    // A cache mapping the paths to already loaded textures
    std::unordered_map<std::string, Texture> m_textures;
};
