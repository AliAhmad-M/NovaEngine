#include <Core/Systems/ScriptSystem.h>
#include <Core/Behavior/BehaviorRegistry.h>
#include <Core/Services.h>
#include <Core/Components/BehaviorScript.h>

using namespace Nova;

ScriptSystem::ScriptSystem() 
{
    m_ecs = Services::Fetch<ECS>();
}

void ScriptSystem::Update(float dt)
{
    for (EntityID entity : entities)
    {
        BehaviorScript* bs = m_ecs->GetComponent<BehaviorScript>(entity);
        if (!bs || !bs->instance) continue;

        if (!bs->started)
        {
            bs->instance->m_ecs = m_ecs;
            bs->instance->m_entity = entity;
            bs->instance->OnStart();
            bs->started = true;
        }

        bs->instance->OnUpdate(dt);
    }
}

void ScriptSystem::OnEntityAdded(EntityID e)
{
    BehaviorScript* bs = m_ecs->GetComponent<BehaviorScript>(e);
    if (!bs || bs->behaviorName.empty()) return;

    bs->instance = BehaviorRegistry::Get().Create(bs->behaviorName);
    bs->started = false;
}

void ScriptSystem::OnEntityRemoved(EntityID e)
{
    BehaviorScript* bs = m_ecs->GetComponent<BehaviorScript>(e);
    if (bs && bs->instance && bs->started)
        bs->instance->OnDestroy();
}

void ScriptSystem::OnAllEntitiesRemoved()
{
    for (EntityID e : entities)
    {
        BehaviorScript* bs = m_ecs->GetComponent<BehaviorScript>(e);
        if (bs && bs->instance && bs->started)
            bs->instance->OnDestroy();
    }
}