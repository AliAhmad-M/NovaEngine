#pragma once
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

public:
    /**
     * @brief Initializes OpenGL buffers, shaders, and the white fallback texture.
     */
    void Init() {
        const char* vs = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            layout(location = 1) in vec2 aTex;
            layout(location = 2) in vec4 aColor;
            uniform mat4 uProj;
            out vec2 vTex;
            out vec4 vColor;
            void main() {
                gl_Position = uProj * vec4(aPos, 0.0, 1.0);
                vTex   = aTex;
                vColor = aColor;
            })";

        const char* fs = R"(
            #version 330 core
            in vec2 vTex;
            in vec4 vColor;
            uniform sampler2D uTex;
            out vec4 fColor;
            void main() {
                fColor = texture(uTex, vTex) * vColor;
            })";

        m_shader = createShaderProgram(vs, fs);

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ibo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, MAX_QUADS * 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        // Texture coordinates
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        // Color
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));

        // Index buffer (0,1,2, 2,3,0)
        std::vector<uint32_t> indices(MAX_QUADS * 6);
        for (uint32_t i = 0, v = 0; i < MAX_QUADS * 6; i += 6, v += 4) {
            indices[i + 0] = v + 0; indices[i + 1] = v + 1; indices[i + 2] = v + 2;
            indices[i + 3] = v + 2; indices[i + 4] = v + 3; indices[i + 5] = v + 0;
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // 1x1 white texture used for solid color draws
        glGenTextures(1, &m_whiteTexture);
        glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
        uint32_t white = 0xFFFFFFFF;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    /**
     * @brief Begins a new render frame with the given projection matrix.
     * @param projection Projection matrix (e.g. orthographic).
     */
    void Begin(const Matrix4x4& projection) {
        glUseProgram(m_shader);
        GLint loc = glGetUniformLocation(m_shader, "uProj");
        glUniformMatrix4fv(loc, 1, GL_FALSE, &projection.m11);
        m_vertexBatch.clear();
    }

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
        float tx, float ty, float tw, float th, const Color& color = Color::WHITE)
    {
        pushQuad(texture, pos, size, rotation, tx, ty, tw, th, color);
    }

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
        const Color& color, bool outline = false, float thickness = 2.0f)
    {
        if (!outline)
        {
            pushQuad(m_whiteTexture, pos, size, rotation, 0, 0, 1, 1, color);
        }
        else
        {
            // Top
            pushQuad(m_whiteTexture, { pos.x, pos.y }, { size.x, thickness }, rotation, 0, 0, 1, 1, color);
            // Bottom
            pushQuad(m_whiteTexture, { pos.x, pos.y + size.y - thickness }, { size.x, thickness }, rotation, 0, 0, 1, 1, color);
            // Left
            pushQuad(m_whiteTexture, { pos.x, pos.y }, { thickness, size.y }, rotation, 0, 0, 1, 1, color);
            // Right
            pushQuad(m_whiteTexture, { pos.x + size.x - thickness, pos.y }, { thickness, size.y }, rotation, 0, 0, 1, 1, color);
        }
    }

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
        bool outline = false, int segments = 32, float thickness = 2.0f)
    {
        float angleStep = (2.0f * 3.14159265f) / (float)segments;

        if (!outline)
        {
            // Fill
            for (int i = 0; i < segments; i++)
            {
                float angle = i * angleStep;
                float nextAngle = (i + 1) * angleStep;

                // Get points on the circumference
                float ax = cosf(angle) * radius;
                float ay = sinf(angle) * radius;
                float bx = cosf(nextAngle) * radius;
                float by = sinf(nextAngle) * radius;

                // Find the midpoint of this outer slice segment
                float mx = (ax + bx) * 0.5f;
                float my = (ay + by) * 0.5f;

                // Calculate distance from center to slice midpoint
                float distToMid = sqrtf(mx * mx + my * my);

                // Calculate width of the outer crust segment
                float dx = bx - ax;
                float dy = by - ay;
                float sliceWidth = sqrtf(dx * dx + dy * dy);

                // Angle matching the direction pointing outwards from center
                float renderAngle = atan2f(my, mx) * (180.0f / 3.14159265f);

                // Position the quad so it anchors exactly at the center point and stretches out
                Vector2 quadPos = {
                    center.x + (mx * 0.5f) - (sliceWidth * 0.5f),
                    center.y + (my * 0.5f) - (distToMid * 0.5f)
                };

                pushQuad(m_whiteTexture,
                    quadPos,
                    { sliceWidth, distToMid },
                    renderAngle, 0, 0, 1, 1, color
                );
            }
        }
        else
        {
            // Outline
            for (int i = 0; i < segments; i++)
            {
                float angle = i * angleStep;
                float nextAngle = (i + 1) * angleStep;

                Vector2 p1 = { center.x + cosf(angle) * radius,     center.y + sinf(angle) * radius };
                Vector2 p2 = { center.x + cosf(nextAngle) * radius, center.y + sinf(nextAngle) * radius };

                // Calculate length of the line segment chord
                float edgeX = p2.x - p1.x;
                float edgeY = p2.y - p1.y;
                float length = sqrtf(edgeX * edgeX + edgeY * edgeY);

                if (length == 0.0f) continue;

                // Find rotation angle to orient the strip towards the next point
                float renderAngle = atan2f(edgeY, edgeX) * (180.0f / 3.14159265f);

                // Find the exact center position of the line segment strip
                Vector2 midPoint = { (p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f };

                // Position the quad so it centers directly over the midpoint of our line slice
                Vector2 quadPos = {
                    midPoint.x - length * 0.5f,
                    midPoint.y - thickness * 0.5f
                };

                pushQuad(m_whiteTexture,
                    quadPos,
                    { length, thickness },
                    renderAngle, 0, 0, 1, 1, color
                );
            }
        }
    }

    /**
     * @brief Flushes all batched quads to the GPU and draws them.
     */
    void Flush() {
        if (m_vertexBatch.empty()) return;

        glBindTexture(GL_TEXTURE_2D, m_currentTexture);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexBatch.size() * sizeof(Vertex), m_vertexBatch.data());

        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)((m_vertexBatch.size() / 4) * 6), GL_UNSIGNED_INT, 0);

        m_vertexBatch.clear();
    }
};