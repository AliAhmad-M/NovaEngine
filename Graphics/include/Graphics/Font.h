#pragma once
#include <glad/glad.h>
#include <unordered_map>
#include <string>
#include <Core/Utilities/Vector2.h>

/**
 * @brief Layout sizing and positioning metadata for a single font character glyph.
 */
struct GlyphInfo {
    Vector2 uvMin;    // Top-left texture coordinate on the font atlas sheet
    Vector2 uvMax;    // Bottom-right texture coordinate on the font atlas sheet
    Vector2 size;     // Absolute width and height of the glyph bounding box in pixels
    Vector2 bearing;  // Horizontal and vertical offset relative to the baseline
    float advance;    // Horizontal step distance required to move to the next character
};

/**
 * @brief Resource container holding the GPU texture atlas and character spacing metrics.
 */
class Font {
public:
    /**
     * @brief Instantiates a blank font asset context configuration.
     */
    Font() = default;

    /**
     * @brief Cleans up graphics resources by deleting the underlying OpenGL texture from VRAM.
     */
    ~Font() { if (m_atlasTexture != 0) glDeleteTextures(1, &m_atlasTexture); }

    /**
     * @brief Parses a TrueType/OpenType font file from disk and builds a packed OpenGL texture atlas.
     * @param path System route leading to the font file (.ttf or .otf).
     * @param fontSize Target vertical pixel size for rasterization.
     * @return true if the font successfully loaded and cached, false otherwise.
     */
    bool LoadFromFile(const std::string& path, float fontSize);

    /**
     * @brief Retrieves the raw OpenGL texture handle identifier.
     * @return Hardware texture ID bound to the atlas sheet.
     */
    GLuint GetTextureHandle() const { return m_atlasTexture; }

    /**
     * @brief Queries metrics for a given ASCII symbol.
     * @param c Target character to look up.
     * @return Pointer to glyph metrics record, or nullptr if the character layout is missing.
     */
    const GlyphInfo* GetGlyph(char c) const {
        auto it = m_glyphs.find(c);
        return it != m_glyphs.end() ? &it->second : nullptr;
    }

private:
    GLuint m_atlasTexture = 0;                     // OpenGL hardware texture object handle
    std::unordered_map<char, GlyphInfo> m_glyphs;  // Dictionary translating characters to font metrics
};