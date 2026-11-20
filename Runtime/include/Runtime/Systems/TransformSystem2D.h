#pragma once

#include <Core/ECS/ECS.h>
#include <Core/Components/Transform2D.h>
#include <Core/Components/WorldTransform2D.h>
#include <unordered_set>

namespace Nova
{
    struct TransformSystem2D : public ISystem
    {
        TransformSystem2D();
        void Update(float dt = 0.0f) override;
        void OnEntityAdded(EntityID e) override;
        void OnEntityRemoved(EntityID e) override;
        void OnAllEntitiesRemoved() override;

    private:
        ECS* m_ecsRef = nullptr;
        std::unordered_set<EntityID> m_warnedEntities;
    };
}
