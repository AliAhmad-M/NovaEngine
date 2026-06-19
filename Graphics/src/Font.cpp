#include <Graphics/Font.h>
#include <Core/EngineDebug.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <vector>

bool Font::LoadFromFile(const std::string& path, float fontSize) 
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) 
    {
        EngineDebug::LogWarn("Cannot load font: %s", path.c_str());
        return false;
    
    } 

    FT_Face face;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) 
    {
        FT_Done_FreeType(ft);
        EngineDebug::LogWarn("Cannot load font: %s", path.c_str());
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)(fontSize));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned int totalWidth = 0;
    unsigned int maxHeight = 0;
    for (unsigned char c = 32; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
        totalWidth += face->glyph->bitmap.width;
        maxHeight = std::max(maxHeight, face->glyph->bitmap.rows);
    }

    // Allocate an RGBA buffer
    std::vector<uint8_t> rgbaBuffer(totalWidth * maxHeight * 4, 0);

    int xOffset = 0;
    for (unsigned char c = 32; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;

        FT_Bitmap& bitmap = face->glyph->bitmap;

        // Blit single-channel bitmap into the master 4-channel RGBA array
        for (unsigned int row = 0; row < bitmap.rows; ++row) {
            for (unsigned int col = 0; col < bitmap.width; ++col) {
                uint8_t alphaValue = bitmap.buffer[row * bitmap.width + col];

                size_t rgbaIndex = ((row * totalWidth) + (xOffset + col)) * 4;
                rgbaBuffer[rgbaIndex + 0] = 255;        // R
                rgbaBuffer[rgbaIndex + 1] = 255;        // G
                rgbaBuffer[rgbaIndex + 2] = 255;        // B
                rgbaBuffer[rgbaIndex + 3] = alphaValue; // A
            }
        }

        GlyphInfo glyph;
        glyph.uvMin = Vector2((float)xOffset / (float)totalWidth, 0.0f);
        glyph.uvMax = Vector2((float)(xOffset + bitmap.width) / (float)totalWidth, (float)bitmap.rows / (float)maxHeight);
        glyph.size  = Vector2((float)bitmap.width, (float)bitmap.rows);
        glyph.bearing = Vector2((float)face->glyph->bitmap_left, (float)face->glyph->bitmap_top);
        glyph.advance = (float)(face->glyph->advance.x >> 6);

        m_glyphs[c] = glyph;
        xOffset += bitmap.width;
    }

    // Generate single standard Texture container 
    glGenTextures(1, &m_atlasTexture);
    glBindTexture(GL_TEXTURE_2D, m_atlasTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, totalWidth, maxHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuffer.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    return true;
}

const GlyphInfo* Font::GetGlyph(char c) const
{
    auto it = m_glyphs.find(c);
    return it != m_glyphs.end() ? &it->second : nullptr;
}

void Font::Destroy() 
{
    if (m_atlasTexture != 0) {
        glDeleteTextures(1, &m_atlasTexture);
        m_atlasTexture = 0;
    }
    m_glyphs.clear();
}