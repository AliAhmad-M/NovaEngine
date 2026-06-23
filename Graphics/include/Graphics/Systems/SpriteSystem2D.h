#pragma once

#include <Core/ECS/ECS.h>
#include <Core/Components/Transform2D.h>
#include <Platform/Window.h>
#include <Graphics/Components/Sprite2D.h>
#include <Graphics/Components/Camera2D.h>
#include <Graphics/Renderer.h>

namespace Nova 
{
    struct SpriteSystem2D : public ISystem
    {
        SpriteSystem2D();
        void Update(float dt = 0.0f) override;
        void OnEntityAdded(EntityID e) override;
        void OnEntityRemoved(EntityID e) override;
        void OnAllEntitiesRemoved() override;

        // Flag to toggle Y sort
        bool enableYSort = false;

    private:
        Window* m_window;
        Renderer* m_renderer;
        Camera2D* m_camera2D;
        ECS* m_ecs;

        std::vector<EntityID> m_entities;
        bool zSortingNeeded = false;

        void sortRenderingOrder();
        float getGlobalYSortOrigin(Transform2D* transform, Sprite2D* sprite);
    };
}

