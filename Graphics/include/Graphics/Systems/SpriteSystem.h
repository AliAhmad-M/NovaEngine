#pragma once
#include <Core/ECS/ECS.h>
#include <Core/Components.h>
#include <Platform/Window.h>
#include <Graphics/Renderer.h>
#include <Graphics/Camera/ICamera.h>
#include <Graphics/Camera/Camera2D.h>
#include <Graphics/Camera/Camera3D.h>
#include <Graphics/Components.h>

struct SpriteSystem : public ISystem {
    void OnEntityAdded(EntityID e) override;
    void OnEntityRemoved(EntityID e) override;
    void OnAllEntitiesRemoved() override;

    void Draw(ICamera* camera, Window* window, Renderer* renderer, ECS* ecs);

    // Flag to toggle Y sort
    bool enableYSort = false;

private:
    bool zSortingNeeded = false;

    void sortRenderingOrder(ECS* ecs);
    float getGlobalYSortOrigin(Transform2D* transform, Sprite2D* sprite);

    std::vector<EntityID> m_entities;
};