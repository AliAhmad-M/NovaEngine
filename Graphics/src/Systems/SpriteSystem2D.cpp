#include <Graphics/Systems/SpriteSystem2D.h>
#include <Core/Services.h>
#include <Core/Utilities/Matrix4x4.h>
#include <Core/Debug.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace Nova;

static const float CAMERA_MARGIN = 32.0f;

// Largest expected world space Y
static const float MAX_Y_RANGE = 100000.0f;

SpriteSystem2D::SpriteSystem2D()
{
    m_windowRef = Services::Fetch<Window>();
    m_rendererRef = Services::Fetch<Renderer>();
    m_ecsRef = Services::Fetch<ECS>();
    m_camera2DRef = Services::Has<Camera2D>() ? Services::Fetch<Camera2D>() : nullptr;
}

void SpriteSystem2D::Update(float dt)
{
    m_camera2DRef = Services::Has<Camera2D>() ? Services::Fetch<Camera2D>() : nullptr;

    int windowWidth, windowHeight;
    m_windowRef->GetSize(&windowWidth, &windowHeight);

    Vector2 camOffset = m_camera2DRef ? m_camera2DRef->GetOffset() : Vector2::ZERO;
    float zoom = m_camera2DRef ? m_camera2DRef->GetZoom() : 1.0f;

    float cullLeft = camOffset.x - CAMERA_MARGIN;
    float cullRight = camOffset.x + (windowWidth / zoom) + CAMERA_MARGIN;
    float cullTop = camOffset.y - CAMERA_MARGIN;
    float cullBottom = camOffset.y + (windowHeight / zoom) + CAMERA_MARGIN;

    glClear(GL_DEPTH_BUFFER_BIT);

    for (size_t i = 0; i < m_entities.size(); i++)
    {
        EntityID e = m_entities[i];
        auto* t = m_ecsRef->GetComponent<Transform2D>(e);
        auto* s = m_ecsRef->GetComponent<Sprite2D>(e);

        if (!s->texture) continue;

        GLuint texHandle = s->texture->GetHandle();
        int texWidth = s->GetTextureWidth();
        int texHeight = s->GetTextureHeight();
        int frameW = s->GetFrameWidth();
        int frameH = s->GetFrameHeight();

        float worldW = frameW * t->scale.x;
        float worldH = frameH * t->scale.y;

        if (t->position.x + worldW < cullLeft ||
            t->position.x > cullRight ||
            t->position.y + worldH < cullTop ||
            t->position.y > cullBottom) continue;

        if (s->GetAnimation() != "none")
        {
            Sprite2DAnimationData& animData = s->animations[s->GetAnimation()];
            size_t frameIndex = (size_t)(glfwGetTime() * animData.speed) % animData.frames.size();
            int frame = animData.frames[frameIndex];

            s->frame = frame;
            s->src.SetCoords(
                (float)((frame % s->hframes) * frameW),
                (float)((frame / s->hframes) * frameH));
        }

        s->src.SetDimensions((float)frameW, (float)frameH);

        float screenX = (t->position.x - camOffset.x) * zoom;
        float screenY = (t->position.y - camOffset.y) * zoom;
        float screenW = worldW * zoom;
        float screenH = worldH * zoom;

        s->dst.SetCoords(screenX, screenY);
        s->dst.SetDimensions(screenW, screenH);

        float u = s->src.GetX() / texWidth;
        float v = s->src.GetY() / texHeight;
        float uw = (float)frameW / texWidth;
        float vh = (float)frameH / texHeight;

        if (s->fliph) { u += uw; uw = -uw; }
        if (s->flipv) { v += vh; vh = -vh; }

        // Sprite rendering depth
        float depth = (float)s->zIndex;
        if (s->applyYSort)
        {
            float ySortOrigin = t->position.y + (worldH / 2.0f) + s->ySortOrigin;
            depth += (ySortOrigin / MAX_Y_RANGE);
        }

        m_rendererRef->DrawQuad(
            texHandle,
            { screenX, screenY },
            { screenW, screenH },
            t->rotation,
            u, v, uw, vh,
            Color::WHITE,
            depth
        );
    }
}

void SpriteSystem2D::OnEntityAdded(EntityID e)
{
    m_entities.push_back(e);
}

void SpriteSystem2D::OnEntityRemoved(EntityID e)
{
    auto position = std::find(m_entities.begin(), m_entities.end(), e);
    m_entities.erase(position);
}

void SpriteSystem2D::OnAllEntitiesRemoved()
{
    m_entities.clear();
}