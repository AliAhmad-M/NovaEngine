#include <Graphics/Texture.h>
#include <Core/Debug.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>

using namespace Nova;

bool Texture::LoadFromFile(const std::string& path) 
{
    // Free old texture if reloading
    Destroy();

    int channels;
    m_pixels = stbi_load(path.c_str(), &m_width, &m_height, &channels, 4);
    if (!m_pixels) 
    {
        Debug::LogWarn("Failed to load texture: %s", path.c_str());
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

bool Texture::CreateRenderTexture(int width, int height) 
{
    Destroy();
    m_width = width;
    m_height = height;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_id, 0);

    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return ok;
}

void Texture::BindFBO() 
{
    glGetIntegerv(GL_VIEWPORT, m_savedViewport);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void Texture::UnbindFBO() 
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(m_savedViewport[0], m_savedViewport[1], m_savedViewport[2], m_savedViewport[3]);
}

void Texture::ClearFBO() 
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Texture::Destroy() 
{
    if (m_fbo) 
    { 
        glDeleteFramebuffers(1, &m_fbo); 
        m_fbo = 0; 
    }

    if (m_id) 
    {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }

    // Only free if not already freed
    if (m_pixels) 
    {
        stbi_image_free(m_pixels);
        m_pixels = nullptr;
    }

    m_width = m_height = 0;
}