#pragma once
#include <Core/ECS/ECS.h>
#include <Core/Components.h>
#include <Graphics/Components.h>

struct SpriteSystem2D : public ISystem 
{
    void Update(float dt = 0.0f) override;
    void OnEntityAdded(EntityID e) override;
    void OnEntityRemoved(EntityID e) override;
    void OnAllEntitiesRemoved() override;

    // Flag to toggle Y sort
    bool enableYSort = false;

private:
    std::vector<EntityID> m_entities;
    bool zSortingNeeded = false;

    void sortRenderingOrder();
    float getGlobalYSortOrigin(Transform2D* transform, Sprite2D* sprite);
};