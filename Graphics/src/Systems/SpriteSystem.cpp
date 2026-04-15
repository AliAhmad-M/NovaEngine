#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Core/Utilities/Matrix4x4.h>
#include <Graphics/Systems/SpriteSystem.h>

#define CAMERA_MARGIN 32

void SpriteSystem::Draw(ICamera* camera, Window* window, Renderer* renderer, ECS* ecs) {
    if (camera == nullptr) {
        fprintf(stderr, "[ERROR]: Camera is NULL!\n");
        exit(1);
    }
    if (window == nullptr) {
        fprintf(stderr, "[ERROR]: Window is NULL!\n");
        exit(1);
    }
    if (renderer == nullptr) {
        fprintf(stderr, "[ERROR]: Renderer is NULL!\n");
        exit(1);
    }
    if (ecs == nullptr) {
        fprintf(stderr, "[ERROR]: ECS is NULL!\n");
        exit(1);
    }

    Camera2D* camera2D = static_cast<Camera2D*>(camera);

    // Get window size
    int windowWidth, windowHeight;
    window->GetSize(&windowWidth, &windowHeight);

    // Get camera offset
    Vector2 cameraOffset = camera2D->GetOffset();

    // Calculate culling bounds
    float cullLeft = cameraOffset.x - CAMERA_MARGIN;
    float cullRight = cameraOffset.x + windowWidth + CAMERA_MARGIN;
    float cullTop = cameraOffset.y - CAMERA_MARGIN;
    float cullBottom = cameraOffset.y + windowHeight + CAMERA_MARGIN;

    for (size_t i = 0; i < m_entities.size(); i++) {
        EntityID e = m_entities[i];
        Transform2D* transform = ecs->GetComponent<Transform2D>(e);
        Sprite2D* sprite = ecs->GetComponent<Sprite2D>(e);

        // Frustrum culling check
        float scaledWidth = sprite->GetFrameWidth() * transform->scaleX;
        float scaledHeight = sprite->GetFrameHeight() * transform->scaleY;

        if (transform->position.x + scaledWidth < cullLeft ||
            transform->position.x > cullRight ||
            transform->position.y + scaledHeight < cullTop ||
            transform->position.y > cullBottom) {
            continue;
        }

        // Texture properties
        GLuint texHandle = sprite->texture->GetHandle();
        int texWidth = sprite->GetTextureWidth();
        int texHeight = sprite->GetTextureHeight();
        int frameW = sprite->GetFrameWidth();
        int frameH = sprite->GetFrameHeight();

        // Handle animation
        if (sprite->GetAnimation() != "none") {
            Sprite2DAnimationData& animData = sprite->animations[sprite->GetAnimation()];
            size_t frameIndex = (size_t)(glfwGetTime() * animData.speed) % animData.frames.size();
            int frame = animData.frames[frameIndex];

            sprite->frame = frame;
            sprite->src.x = (float)((frame % sprite->hframes) * frameW);
            sprite->src.y = (float)((frame / sprite->hframes) * frameH);
        }

        // Set source rect
        sprite->src.w = (float)frameW;
        sprite->src.h = (float)frameH;

        // Set destination rect
        sprite->dst.x = transform->position.x - cameraOffset.x;
        sprite->dst.y = transform->position.y - cameraOffset.y;
        sprite->dst.w = (float)(frameW * transform->scaleX);
        sprite->dst.h = (float)(frameH * transform->scaleY);

        // Calculate Normalized UV coordinates (0.0 to 1.0)
        float u = sprite->src.x / texWidth;
        float v = sprite->src.y / texHeight;
        float uw = (float)frameW / texWidth;
        float vh = (float)frameH / texHeight;

        // Handle Flips by adjusting UVs
        if (sprite->fliph) { u += uw; uw = -uw; }
        if (sprite->flipv) { v += vh; vh = -vh; }

        // Draw the sprites
        renderer->DrawQuad(
            sprite->texture->GetHandle(),
            { transform->position.x - cameraOffset.x, transform->position.y - cameraOffset.y },
            { scaledWidth, scaledHeight },
            transform->rotation,
            u, v, uw, vh
        );
    }

    if (enableYSort || zSortingNeeded) sortRenderingOrder(ecs);
}

void SpriteSystem::OnEntityAdded(EntityID e) {
    // Add the entity, sort the layers
    m_entities.push_back(e);
    zSortingNeeded = true;
}

void SpriteSystem::OnEntityRemoved(EntityID e) {
    // Position of entity in vector
    auto position = std::find(
        m_entities.begin(),
        m_entities.end(), e
    );

    // Delete the entity
    m_entities.erase(position);
}

void SpriteSystem::OnAllEntitiesRemoved() { m_entities.clear(); }

void SpriteSystem::sortRenderingOrder(ECS* ecs) {
    // Insertion sort entities based on Z indexes and Y sort origins
    for (size_t i = 1; i < m_entities.size(); i++) {
        EntityID key = m_entities[i];

        auto* transformA = ecs->GetComponent<Transform2D>(key);
        auto* spriteA = ecs->GetComponent<Sprite2D>(key);

        float ySortOriginA = getGlobalYSortOrigin(transformA, spriteA);

        int j = (int)i - 1;
        while (j >= 0) {
            auto* transformB = ecs->GetComponent<Transform2D>(m_entities[j]);
            auto* spriteB = ecs->GetComponent<Sprite2D>(m_entities[j]);

            float ySortOriginB = getGlobalYSortOrigin(transformB, spriteB);

            bool move = false;

            // Compare Z
            if (spriteB->GetZIndex() > spriteA->GetZIndex()) {
                move = true;
            }

            // Compare Y
            else if (spriteB->GetZIndex() == spriteA->GetZIndex() &&
                enableYSort && spriteA->applyYSort && spriteB->applyYSort) {
                if (ySortOriginB > ySortOriginA) {
                    move = true;
                }
            }

            if (!move) break;

            m_entities[j + 1] = m_entities[j];
            j--;
        }
        m_entities[j + 1] = key;
    }

    zSortingNeeded = false;
}

float SpriteSystem::getGlobalYSortOrigin(Transform2D* transform, Sprite2D* sprite) {
    Vector2 position = transform->position;
    float ySortOrigin = sprite->ySortOrigin;

    // Calculate the scaled height of the sprite frame
    int textureHeight = sprite->GetTextureHeight();
    float scaledFrameHeight = (textureHeight / sprite->vframes) * transform->scaleY;

    // Calculate global coordinates of the y-sort point
    float y = position.y + scaledFrameHeight / 2 + ySortOrigin;

    return y;
}