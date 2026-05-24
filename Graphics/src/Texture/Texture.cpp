#include <Graphics/Texture/Texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>
#include <cstdio>

Texture::Texture() = default;

bool Texture::LoadFromFile(const std::string& path) {
    // Free old texture if reloading
    Destroy();

    int channels;
    m_pixels = stbi_load(path.c_str(), &m_width, &m_height, &channels, 4);
    if (!m_pixels) {
        fprintf(stderr, "[WARN]: Failed to load texture: %s\n", path.c_str());
        return false;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels);

    // Free data after uploading to GPU
    stbi_image_free(m_pixels);
    m_pixels = nullptr;

    return true;
}

void Texture::Destroy() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }

    // Only free if not already freed
    if (m_pixels) {
        stbi_image_free(m_pixels);
        m_pixels = nullptr;
    }

    m_width = m_height = 0;
}