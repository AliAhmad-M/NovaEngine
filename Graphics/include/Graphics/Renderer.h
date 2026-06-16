#pragma once
#include "Font.h"
#include <vector>
#include <cmath>
#include <glad/glad.h>

#include <Core/Utilities/Vector2.h>
#include <Core/Utilities/Matrix4x4.h>
#include <Core/Utilities/Color.h>

/**
 * @brief A single vertex with position, texture coordinates, and color.
 */
struct Vertex {
    Vector2 position;
    Vector2 texCoord;
    float r, g, b, a; // Normalized color (0.0 - 1.0)
};

class Renderer {
public:
    /**
     * @brief Initializes OpenGL buffers, shaders, and the white fallback texture.
     */
    void Init();

    /**
     * @brief Begins a new render frame with the given projection matrix.
     * @param projection Projection matrix (e.g. orthographic).
     */
    void Begin(const Matrix4x4& projection);

    void DrawLine(Vector2 a, Vector2 b, const Color& color, float thickness = 2.0f);

    /**
     * @brief Draws a textured and tinted quad.
     * @param texture  OpenGL texture ID.
     * @param pos      Top-left position.
     * @param size     Width and height.
     * @param rotation Rotation in degrees.
     * @param tx       Texture coordinate X offset (normalized).
     * @param ty       Texture coordinate Y offset (normalized).
     * @param tw       Texture coordinate width (normalized).
     * @param th       Texture coordinate height (normalized).
     * @param color    Tint color applied to the texture sample.
     */
    void DrawQuad(GLuint texture, Vector2 pos, Vector2 size, float rotation,
        float tx, float ty, float tw, float th, const Color& color = Color::WHITE);

    /**
     * @brief Draws a rectangle, either filled or as a wireframe outline.
     * @param pos       Top-left position of the rectangle.
     * @param size      Width and height of the rectangle.
     * @param rotation  Rotation in degrees.
     * @param color     Fill or outline color.
     * @param outline   If true, draws only the border. If false, draws filled.
     * @param thickness Border thickness in pixels (only used when outline=true).
     */
    void DrawRect(Vector2 pos, Vector2 size, float rotation,
        const Color& color, bool outline = false, float thickness = 2.0f);

    /**
     * @brief Draws a circle, either filled or as a wireframe outline.
     * @param center    World-space center of the circle.
     * @param radius    Radius in pixels.
     * @param color     Fill or outline color.
     * @param outline   If true, draws only the border. If false, draws filled.
     * @param segments  Number of segments used to approximate the circle (higher = smoother).
     * @param thickness Segment quad size in pixels (only used when outline=true).
     */
    void DrawCircle(Vector2 center, float radius, const Color& color,
        bool outline = false, int segments = 32, float thickness = 2.0f);

    /**
     * @brief Draws a string of text in a given font
     * @param font        The pre-loaded Font asset resource containing the texture atlas and glyph metrics.
     * @param text        The string sequence to be rasterized and displayed on screen.
     * @param pos         The initial top-left anchor point or baseline starting coordinate for the text layout block.
     * @param scale       Visual uniform magnification factor (1.0f represents native font pixel sizing).
     * @param color       RGBA color multiplier to dynamically tint the text geometry (defaults to opaque absolute white).
     */
    void DrawText(const Font& font, const std::string& text, Vector2 pos, float scale, const Color& color = Color::WHITE);

    /**
     * @brief Flushes all batched quads to the GPU and draws them.
     */
    void Flush();

private:
    GLuint m_vao = 0, m_vbo = 0, m_ibo = 0;

    GLuint m_shader = 0;
    GLuint m_whiteTexture = 0;
    GLuint m_currentTexture = 0;

    std::vector<Vertex> m_vertexBatch;
    static const size_t MAX_QUADS = 2000;

    /**
     * @brief Compiles and links a vertex and fragment shader into a program.
     * @param vertexSrc   GLSL source for the vertex shader.
     * @param fragmentSrc GLSL source for the fragment shader.
     * @return Linked shader program ID.
     */
    GLuint createShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
        auto compile = [](GLenum type, const char* source) {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);
            return shader;
            };

        GLuint vs = compile(GL_VERTEX_SHADER, vertexSrc);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);
        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        glDeleteShader(vs);
        glDeleteShader(fs);
        return program;
    }

    /**
     * @brief Pushes four vertices for a quad into the batch, with rotation and color.
     * @param texture  OpenGL texture ID to use.
     * @param pos      Top-left position of the quad.
     * @param size     Width and height of the quad.
     * @param rotation Rotation in degrees.
     * @param tx       Texture coordinate X offset.
     * @param ty       Texture coordinate Y offset.
     * @param tw       Texture coordinate width.
     * @param th       Texture coordinate height.
     * @param color    Tint color (bytes 0-255).
     */
    void pushQuad(GLuint texture, Vector2 pos, Vector2 size, float rotation,
        float tx, float ty, float tw, float th, const Color& color)
    {
        if (texture != m_currentTexture || m_vertexBatch.size() + 4 > MAX_QUADS * 4) {
            Flush();
            m_currentTexture = texture;
        }

        // Normalize color bytes to 0.0 - 1.0
        float r = color.r / 255.0f;
        float g = color.g / 255.0f;
        float b = color.b / 255.0f;
        float a = color.a / 255.0f;

        float rad = rotation * (3.14159f / 180.0f);
        float cosR = cosf(rad);
        float sinR = sinf(rad);
        Vector2 halfSize = { size.x * 0.5f, size.y * 0.5f };

        auto getRot = [&](float x, float y) {
            float rx = x * cosR - y * sinR;
            float ry = x * sinR + y * cosR;
            return Vector2{ rx + pos.x + halfSize.x, ry + pos.y + halfSize.y };
            };

        m_vertexBatch.push_back({ getRot(-halfSize.x, -halfSize.y), { tx,      ty      }, r, g, b, a });
        m_vertexBatch.push_back({ getRot(halfSize.x, -halfSize.y), { tx + tw, ty      }, r, g, b, a });
        m_vertexBatch.push_back({ getRot(halfSize.x,  halfSize.y), { tx + tw, ty + th }, r, g, b, a });
        m_vertexBatch.push_back({ getRot(-halfSize.x,  halfSize.y), { tx,      ty + th }, r, g, b, a });
    }
};