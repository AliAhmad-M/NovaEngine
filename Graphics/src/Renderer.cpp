#include <Graphics/Renderer.h>

using namespace Nova;

void Renderer::pushQuad(GLuint texture, Vector2 pos, Vector2 size, float rotation,
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

GLuint Renderer::createShaderProgram(const char* vertexSrc, const char* fragmentSrc)
{
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

void Renderer::Init() {
    const char* vs = 
        R"(#version 330 core
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

    const char* fs = 
        R"(#version 330 core
        in vec2 vTex;
        in vec4 vColor;
        uniform sampler2D uTex;
        out vec4 fColor;
        void main() {
            // Most font libraries output a GL_RED or GL_ALPHA texture. 
            // We sample the texture and apply it directly to the tint color.
            fColor = texture(uTex, vTex) * vColor;
        })";

    m_shader = createShaderProgram(vs, fs);

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ibo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_QUADS * 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));

    std::vector<uint32_t> indices(MAX_QUADS * 6);
    for (uint32_t i = 0, v = 0; i < MAX_QUADS * 6; i += 6, v += 4) {
        indices[i + 0] = v + 0; indices[i + 1] = v + 1; indices[i + 2] = v + 2;
        indices[i + 3] = v + 2; indices[i + 4] = v + 3; indices[i + 5] = v + 0;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenTextures(1, &m_whiteTexture);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    uint32_t white = 0xFFFFFFFF;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Initialize explicit tracking to prevent early flush mismatch
    m_currentTexture = m_whiteTexture;
}

void Renderer::Begin(const Matrix4x4& projection) {
    glUseProgram(m_shader);
    GLint loc = glGetUniformLocation(m_shader, "uProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &projection.m11);

    // Force the shader to use Texture Unit 0
    glUniform1i(glGetUniformLocation(m_shader, "uTex"), 0);

    m_vertexBatch.clear();
}

void Renderer::DrawLine(Vector2 a, Vector2 b, const Color& color, float thickness)
{
    Vector2 dir = { b.x - a.x, b.y - a.y };
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.0001f) return;

    // Angle of the line direction
    float angle = atan2f(dir.y, dir.x) * (180.0f / 3.14159f);

    // Midpoint of the segment
    Vector2 mid = { (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f };
    Vector2 quadPos = { mid.x - len * 0.5f, mid.y - thickness * 0.5f };

    pushQuad(m_whiteTexture, quadPos, { len, thickness }, angle, 0, 0, 1, 1, color);
}

void Renderer::DrawQuad(GLuint texture, Vector2 pos, Vector2 size, float rotation,
    float tx, float ty, float tw, float th, const Color& color)
{
    pushQuad(texture, pos, size, rotation, tx, ty, tw, th, color);
}

void Renderer::DrawRect(Vector2 pos, Vector2 size, float rotation, const Color& color, bool outline, float thickness)
{
    if (!outline)
    {
        pushQuad(m_whiteTexture, pos, size, rotation, 0, 0, 1, 1, color);
        return;
    }

    // Compute actual rotated corners
    float rad = rotation * (3.14159f / 180.0f);
    float cosR = cosf(rad);
    float sinR = sinf(rad);
    float hw = size.x * 0.5f;
    float hh = size.y * 0.5f;

    Vector2 center = { pos.x + hw, pos.y + hh };

    auto rotate = [&](float lx, float ly) -> Vector2 {
        return {
            center.x + lx * cosR - ly * sinR,
            center.y + lx * sinR + ly * cosR
        };
        };

    Vector2 tl = rotate(-hw, -hh);
    Vector2 tr = rotate(hw, -hh);
    Vector2 br = rotate(hw, hh);
    Vector2 bl = rotate(-hw, hh);

    DrawLine(tl, tr, color, thickness);
    DrawLine(tr, br, color, thickness);
    DrawLine(br, bl, color, thickness);
    DrawLine(bl, tl, color, thickness);
}

void Renderer::DrawCircle(Vector2 center, float radius, const Color& color, bool outline, int segments, float thickness)
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

void Renderer::DrawText(const Font& font, const std::string& text, Vector2 pos, float scale, const Color& color)
{
    Vector2 cursor = pos;
    GLuint textureHandle = font.GetTextureHandle();

    for (std::string::const_iterator c = text.begin(); c != text.end(); ++c)
    {
        const GlyphInfo* glyph = font.GetGlyph(*c);
        if (!glyph) continue;

        // Handle space adjustments
        if (*c == ' ') {
            cursor.x += glyph->advance * scale;
            continue;
        }

        // Calculate positions
        float xPos = cursor.x + glyph->bearing.x * scale;
        float yPos = cursor.y - glyph->bearing.y * scale;

        float width = glyph->size.x * scale;
        float height = glyph->size.y * scale;

        // Extract customized UV scaling metrics
        float tx = glyph->uvMin.x;
        float ty = glyph->uvMin.y;
        float tw = glyph->uvMax.x - glyph->uvMin.x;
        float th = glyph->uvMax.y - glyph->uvMin.y;

        pushQuad(textureHandle, { xPos, yPos }, { width, height }, 0.0f, tx, ty, tw, th, color);

        // Step cursor forward
        cursor.x += glyph->advance * scale;
    }
}

void Renderer::Flush() {
    if (m_vertexBatch.empty()) return;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_currentTexture);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexBatch.size() * sizeof(Vertex), m_vertexBatch.data());

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)((m_vertexBatch.size() / 4) * 6), GL_UNSIGNED_INT, 0);

    m_vertexBatch.clear();
}
