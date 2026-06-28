#include <Graphics/Systems/SpriteSystem2D.h>
#include <Core/Services.h>
#include <Core/Utilities/Matrix4x4.h>
#include <Core/Debug.h>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace Nova;

static const float CAMERA_MARGIN = 32.0f;

void SpriteSystem2D::sortRenderingOrder()
{
    ECS* m_ecs = Services::Fetch<ECS>();

    // Insertion sort entities based on Z indexes and Y sort origins
    for (size_t i = 1; i < m_entities.size(); i++)
    {
        EntityID key = m_entities[i];

        auto* transformA = m_ecs->GetComponent<Transform2D>(key);
        auto* spriteA = m_ecs->GetComponent<Sprite2D>(key);

        float ySortOriginA = getGlobalYSortOrigin(transformA, spriteA);

        int j = (int)i - 1;
        while (j >= 0)
        {
            auto* transformB = m_ecs->GetComponent<Transform2D>(m_entities[j]);
            auto* spriteB = m_ecs->GetComponent<Sprite2D>(m_entities[j]);

            float ySortOriginB = getGlobalYSortOrigin(transformB, spriteB);

            bool move = false;

            // Compare Z
            if (spriteB->GetZIndex() > spriteA->GetZIndex())
                move = true;

            // Compare Y
            else if (spriteB->GetZIndex() == spriteA->GetZIndex() &&
                enableYSort && spriteA->applyYSort && spriteB->applyYSort)
            {
                if (ySortOriginB > ySortOriginA)
                    move = true;
            }

            if (!move) break;

            m_entities[j + 1] = m_entities[j];
            j--;
        }
        m_entities[j + 1] = key;
    }

    zSortingNeeded = false;
}

float SpriteSystem2D::getGlobalYSortOrigin(Transform2D* transform, Sprite2D* sprite)
{
    Vector2 position = transform->position;
    float ySortOrigin = sprite->ySortOrigin;

    // Calculate the scaled height of the sprite frame
    int textureHeight = sprite->GetTextureHeight();
    float scaledFrameHeight = (textureHeight / sprite->vframes) * transform->scale.y;

    // Calculate global coordinates of the y-sort point
    float y = position.y + scaledFrameHeight / 2 + ySortOrigin;

    return y;
}

SpriteSystem2D::SpriteSystem2D() 
{
    m_window   = Services::Fetch<Window>();
    m_renderer = Services::Fetch<Renderer>();
    m_ecs      = Services::Fetch<ECS>();
}

void SpriteSystem2D::Update(float dt) 
{
    // Try fetch camera on each update
    m_camera2D = Services::Has<Camera2D>() ? Services::Fetch<Camera2D>() : nullptr;

    // Get m_window size
    int windowWidth, windowHeight;
    m_window->GetSize(&windowWidth, &windowHeight);

    // Get camera offset and zoom
    Vector2 camOffset = m_camera2D ? m_camera2D->GetOffset() : Vector2::ZERO;
    float zoom = m_camera2D ? m_camera2D->GetZoom() : 1.0f;

    // Calculate culling bounds
    float cullLeft = camOffset.x - CAMERA_MARGIN;
    float cullRight = camOffset.x + (windowWidth / zoom) + CAMERA_MARGIN;
    float cullTop = camOffset.y - CAMERA_MARGIN;
    float cullBottom = camOffset.y + (windowHeight / zoom) + CAMERA_MARGIN;

    for (size_t i = 0; i < m_entities.size(); i++) 
    {
        EntityID e = m_entities[i];
        auto* t = m_ecs->GetComponent<Transform2D>(e);
        auto* s = m_ecs->GetComponent<Sprite2D>(e);

        if (!s->texture) continue;

        // Texture properties
        GLuint texHandle = s->texture->GetHandle();
        int texWidth = s->GetTextureWidth();
        int texHeight = s->GetTextureHeight();
        int frameW = s->GetFrameWidth();
        int frameH = s->GetFrameHeight();
        
        // Dimensions of sprite world space
        float worldW = frameW * t->scale.x;
        float worldH = frameH * t->scale.y;

        // Frustum culling check (still in world space)
        if (t->position.x + worldW < cullLeft ||
            t->position.x > cullRight ||
            t->position.y + worldH < cullTop ||
            t->position.y > cullBottom) continue;

        // Handle animation
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

        // Set source rect
        s->src.SetDimensions((float)frameW, (float)frameH);

        // Position of sprite in screen space
        float screenX = (t->position.x - camOffset.x) * zoom;
        float screenY = (t->position.y - camOffset.y) * zoom;

        // Dimensions of sprite in screen space
        float screenW = worldW * zoom;
        float screenH = worldH * zoom;

        // Set destination rect
        s->dst.SetCoords(screenX, screenY);
        s->dst.SetDimensions(screenW, screenH);

        // Calculate normalized UV coordinates
        float u = s->src.GetX() / texWidth;
        float v = s->src.GetY() / texHeight;
        float uw = (float)frameW / texWidth;
        float vh = (float)frameH / texHeight;

        // Handle flips by adjusting UVs
        if (s->fliph) { u += uw; uw = -uw; }
        if (s->flipv) { v += vh; vh = -vh; }

        // Draw the sprites
        m_renderer->DrawQuad(
            s->texture->GetHandle(),
            { screenX, screenY },
            { screenW, screenH },
            t->rotation,
            u, v, uw, vh
        );

        // Check if sorting is needed
        if (s->IsZIndexChanged() && !zSortingNeeded) zSortingNeeded = true;
    }

    if (enableYSort || zSortingNeeded) sortRenderingOrder();
}

void SpriteSystem2D::OnEntityAdded(EntityID e) 
{
    // Add the entity, sort the layers
    m_entities.push_back(e);
    zSortingNeeded = true;
}

void SpriteSystem2D::OnEntityRemoved(EntityID e) 
{
    // Position of entity in vector
    auto position = std::find(
        m_entities.begin(),
        m_entities.end(), e
    );

    // Delete the entity
    m_entities.erase(position);
}

void SpriteSystem2D::OnAllEntitiesRemoved() 
{ 
    m_entities.clear(); 
}

