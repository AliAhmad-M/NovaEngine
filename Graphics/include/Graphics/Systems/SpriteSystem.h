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
    SpriteSystem(ECS* ecs, ICamera* camera, Window* window, Renderer* renderer)
        : m_ecs(ecs), m_camera(camera), m_window(window), m_renderer(renderer) {
    }

    void Update(float dt = 0.0f) override;
    void OnEntityAdded(EntityID e) override;
    void OnEntityRemoved(EntityID e) override;
    void OnAllEntitiesRemoved() override;

    // Flag to toggle Y sort
    bool enableYSort = false;

private:
    ECS* m_ecs = nullptr;
    ICamera* m_camera = nullptr;
    Window* m_window = nullptr;
    Renderer* m_renderer = nullptr;

    bool zSortingNeeded = false;

    void sortRenderingOrder(ECS* ecs);
    float getGlobalYSortOrigin(Transform2D* transform, Sprite2D* sprite);

    std::vector<EntityID> m_entities;
};