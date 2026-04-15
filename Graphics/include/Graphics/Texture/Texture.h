#pragma once
#include <string>
#include <cstdint>

/**
 * @brief 2D texture loaded from an image file.
 */
class Texture {
public:
    /** @brief Creates an empty texture. */
    Texture();

    /**
     * @brief Load texture from an image file (e.g. PNG, JPG).
     * @param path Image file path.
     * @return True on success, false on failure.
     */
    bool LoadFromFile(const std::string& path);

    /** @brief Release texture data and handle. */
    void Destroy();

    /** @brief Texture width in pixels. */
    int GetWidth() const { return m_width; }

    /** @brief Texture height in pixels. */
    int GetHeight() const { return m_height; }

    /** @brief Graphics API texture handle. */
    unsigned int GetHandle() const { return m_id; }

    /** @brief Pointer to raw RGBA pixel data. */
    unsigned char* GetPixels() const { return m_pixels; }

    /** @brief Bytes per row (width × 4). */
    int GetPitch() const { return m_width * 4; }

    /**
     * @brief Width of a single frame in a sprite sheet.
     * @param hframes Number of horizontal frames.
     */
    int GetFrameWidth(int hframes = 1) const { return m_width / hframes; }

    /**
     * @brief Height of a single frame in a sprite sheet.
     * @param vframes Number of vertical frames.
     */
    int GetFrameHeight(int vframes = 1) const { return m_height / vframes; }

private:
    unsigned int m_id = 0;
    int m_width = 0;
    int m_height = 0;
    unsigned char* m_pixels = nullptr;
};
