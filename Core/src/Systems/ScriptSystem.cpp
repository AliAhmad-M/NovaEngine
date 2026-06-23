#include <Core/Systems/ScriptSystem.h>
#include <Core/ECS/ECS.h>
#include <Core/Behavior/BehaviorRegistry.h>
#include <Core/Services.h>
#include <Core/Components/BehaviorScript.h>

using namespace Nova;

void ScriptSystem::Update(float dt)
{
    ECS* ecs = Services::Fetch<ECS>();
    if (!ecs) return;

    for (EntityID entity : entities)
    {
        BehaviorScript* bs = ecs->GetComponent<BehaviorScript>(entity);
        if (!bs || !bs->instance) continue;

        if (!bs->started)
        {
            bs->instance->m_ecs = ecs;
            bs->instance->m_entity = entity;
            bs->instance->OnStart();
            bs->started = true;
        }

        bs->instance->OnUpdate(dt);
    }
}

void ScriptSystem::OnEntityAdded(EntityID e)
{
    ECS* ecs = Services::Fetch<ECS>();
    BehaviorScript* bs = ecs->GetComponent<BehaviorScript>(e);
    if (!bs || bs->behaviorName.empty()) return;

    bs->instance = BehaviorRegistry::Get().Create(bs->behaviorName);
    bs->started = false;
}

void ScriptSystem::OnEntityRemoved(EntityID e)
{
    ECS* ecs = Services::Fetch<ECS>();
    if (!ecs) return;

    BehaviorScript* bs = ecs->GetComponent<BehaviorScript>(e);
    if (bs && bs->instance && bs->started)
        bs->instance->OnDestroy();
}

void ScriptSystem::OnAllEntitiesRemoved()
{
    ECS* ecs = Services::Fetch<ECS>();
    if (!ecs) return;

    for (EntityID e : entities)
    {
        BehaviorScript* bs = ecs->GetComponent<BehaviorScript>(e);
        if (bs && bs->instance && bs->started)
            bs->instance->OnDestroy();
    }
}