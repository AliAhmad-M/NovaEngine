#pragma once
#include <vector>
#include <glad/glad.h>

#include <Core/Utilities/Vector2.h>
#include <Core/Utilities/Matrix4x4.h>

struct Vertex {
    Vector2 position;
    Vector2 texCoord;
};

class Renderer {
private:
    GLuint m_vao = 0, m_vbo = 0, m_ibo = 0;
    
    GLuint m_shader         = 0;
    GLuint m_whiteTexture   = 0;
    GLuint m_currentTexture = 0;

    std::vector<Vertex> m_vertexBatch;
    static const size_t MAX_QUADS = 2000;

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

public:
    void Init() {
        // Vertex and fragment shaders
        const char* vs = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            layout(location = 1) in vec2 aTex;
            uniform mat4 uProj;
            out vec2 vTex;
            void main() { 
                gl_Position = uProj * vec4(aPos, 0.0, 1.0); 
                vTex = aTex; 
            })";

        const char* fs = R"(
            #version 330 core
            in vec2 vTex;
            uniform sampler2D uTex;
            out vec4 fColor;
            void main() { fColor = texture(uTex, vTex); })";

        m_shader = createShaderProgram(vs, fs);

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ibo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, MAX_QUADS * 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0); // Position
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1); // TexCoord
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(Vector2));

        // Index buffer setup (0,1,2, 2,3,0)
        std::vector<uint32_t> indices(MAX_QUADS * 6);
        for (uint32_t i = 0, v = 0; i < MAX_QUADS * 6; i += 6, v += 4) {
            indices[i + 0] = v + 0; indices[i + 1] = v + 1; indices[i + 2] = v + 2;
            indices[i + 3] = v + 2; indices[i + 4] = v + 3; indices[i + 5] = v + 0;
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void Begin(const Matrix4x4& projection) {
        glUseProgram(m_shader);
        GLint loc = glGetUniformLocation(m_shader, "uProj");
        glUniformMatrix4fv(loc, 1, GL_FALSE, &projection.m11);
        m_vertexBatch.clear();
    }

    void DrawQuad(GLuint texture, Vector2 pos, Vector2 size, float rotation, float tx, float ty, float tw, float th) {
        if (texture != m_currentTexture || m_vertexBatch.size() + 4 > MAX_QUADS * 4) {
            Flush();
            m_currentTexture = texture;
        }

        // Rotation
        float rad = rotation * (3.14159f / 180.0f);
        float cosR = cosf(rad);
        float sinR = sinf(rad);
        Vector2 halfSize = { size.x * 0.5f, size.y * 0.5f };

        auto getRot = [&](float x, float y) {
            float rx = x * cosR - y * sinR;
            float ry = x * sinR + y * cosR;
            // Translation
            return Vector2{ rx + pos.x + halfSize.x, ry + pos.y + halfSize.y };
        };

        m_vertexBatch.push_back({ getRot(-halfSize.x, -halfSize.y), {tx, ty} });
        m_vertexBatch.push_back({ getRot(halfSize.x, -halfSize.y), {tx + tw, ty} });
        m_vertexBatch.push_back({ getRot(halfSize.x,  halfSize.y), {tx + tw, ty + th} });
        m_vertexBatch.push_back({ getRot(-halfSize.x,  halfSize.y), {tx, ty + th} });
    }

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