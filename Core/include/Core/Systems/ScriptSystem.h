#pragma once
#include <Core/ECS/ISystem.h>

namespace Nova
{
    class ScriptSystem : public ISystem
    {
    public:
        void Update(float dt)            override;
        void OnEntityAdded(EntityID e)   override;
        void OnEntityRemoved(EntityID e) override;
        void OnAllEntitiesRemoved()      override;
    };
}