#include <Graphics/Systems/SpriteSystem2D.h>
#include <Core/Services.h>
#include <Core/Utilities/Matrix4x4.h>
#include <Core/Debug.h>
#include <Graphics/Components/Camera2D.h>
#include <Graphics/Renderer.h>
#include <Platform/Window.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace Nova;

static const float CAMERA_MARGIN = 32.0f;

void SpriteSystem2D::sortRenderingOrder()
{
    ECS* ecs = Services::Fetch<ECS>();

    // Insertion sort entities based on Z indexes and Y sort origins
    for (size_t i = 1; i < m_entities.size(); i++)
    {
        EntityID key = m_entities[i];

        auto* transformA = ecs->GetComponent<Transform2D>(key);
        auto* spriteA = ecs->GetComponent<Sprite2D>(key);

        float ySortOriginA = getGlobalYSortOrigin(transformA, spriteA);

        int j = (int)i - 1;
        while (j >= 0)
        {
            auto* transformB = ecs->GetComponent<Transform2D>(m_entities[j]);
            auto* spriteB = ecs->GetComponent<Sprite2D>(m_entities[j]);

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

void SpriteSystem2D::Update(float dt) 
{
    Camera2D* camera2D  = Services::Fetch<Camera2D>();
    Window* window      = Services::Fetch<Window>();
    Renderer* renderer  = Services::Fetch<Renderer>();
    ECS* ecs            = Services::Fetch<ECS>();

    // Get window size
    int windowWidth, windowHeight;
    window->GetSize(&windowWidth, &windowHeight);

    // Get camera offset and zoom
    Vector2 camOffset = camera2D ? camera2D->GetOffset() : Vector2::ZERO;
    float zoom = camera2D ? camera2D->GetZoom() : 1.0f;

    // Calculate culling bounds
    float cullLeft = camOffset.x - CAMERA_MARGIN;
    float cullRight = camOffset.x + (windowWidth / zoom) + CAMERA_MARGIN;
    float cullTop = camOffset.y - CAMERA_MARGIN;
    float cullBottom = camOffset.y + (windowHeight / zoom) + CAMERA_MARGIN;

    for (size_t i = 0; i < m_entities.size(); i++) 
    {
        EntityID e = m_entities[i];
        Transform2D* transform = ecs->GetComponent<Transform2D>(e);
        Sprite2D* sprite = ecs->GetComponent<Sprite2D>(e);

        // Texture properties
        GLuint texHandle = sprite->texture->GetHandle();
        int texWidth = sprite->GetTextureWidth();
        int texHeight = sprite->GetTextureHeight();
        int frameW = sprite->GetFrameWidth();
        int frameH = sprite->GetFrameHeight();
        
        // Dimensions of sprite world space
        float worldW = frameW * transform->scale.x;
        float worldH = frameH * transform->scale.y;

        // Frustum culling check (still in world space)
        if (transform->position.x + worldW < cullLeft ||
            transform->position.x > cullRight ||
            transform->position.y + worldH < cullTop ||
            transform->position.y > cullBottom) continue;

        // Handle animation
        if (sprite->GetAnimation() != "none") 
        {
            Sprite2DAnimationData& animData = sprite->animations[sprite->GetAnimation()];
            size_t frameIndex = (size_t)(glfwGetTime() * animData.speed) % animData.frames.size();
            int frame = animData.frames[frameIndex];

            sprite->frame = frame;
            sprite->src.SetCoords(
                (float)((frame % sprite->hframes) * frameW), 
                (float)((frame / sprite->hframes) * frameH));
        }

        // Set source rect
        sprite->src.SetDimensions((float)frameW, (float)frameH);

        // Position of sprite in screen space
        float screenX = (transform->position.x - camOffset.x) * zoom;
        float screenY = (transform->position.y - camOffset.y) * zoom;

        // Dimensions of sprite in screen space
        float screenW = worldW * zoom;
        float screenH = worldH * zoom;

        // Set destination rect
        sprite->dst.SetCoords(screenX, screenY);
        sprite->dst.SetDimensions(screenW, screenH);

        // Calculate normalized UV coordinates
        float u = sprite->src.GetX() / texWidth;
        float v = sprite->src.GetY() / texHeight;
        float uw = (float)frameW / texWidth;
        float vh = (float)frameH / texHeight;

        // Handle flips by adjusting UVs
        if (sprite->fliph) { u += uw; uw = -uw; }
        if (sprite->flipv) { v += vh; vh = -vh; }

        // Draw the sprites
        renderer->DrawQuad(
            sprite->texture->GetHandle(),
            { screenX, screenY },
            { screenW, screenH },
            transform->rotation,
            u, v, uw, vh
        );
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

