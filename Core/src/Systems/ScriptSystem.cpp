#include <Core/Systems/ScriptSystem.h>
#include <Core/Behavior/BehaviorRegistry.h>
#include <Core/Services.h>
#include <Core/Components/BehaviorScript.h>

using namespace Nova;

ScriptSystem::ScriptSystem() 
{
    m_ecsRef = Services::Fetch<ECS>();
}

void ScriptSystem::Update(float dt)
{
    for (EntityID entity : entities)
    {
        BehaviorScript* bs = m_ecsRef->GetComponent<BehaviorScript>(entity);
        if (!bs || !bs->instance) continue;

        if (!bs->started)
        {
            bs->instance->m_ecsRef = m_ecsRef;
            bs->instance->m_entity = entity;
            bs->instance->OnStart();
            bs->started = true;
        }

        bs->instance->OnUpdate(dt);
    }
}

void ScriptSystem::OnEntityAdded(EntityID e)
{
    BehaviorScript* bs = m_ecsRef->GetComponent<BehaviorScript>(e);
    if (!bs || bs->behaviorName.empty()) return;

    bs->instance = BehaviorRegistry::Get().Create(bs->behaviorName);
    bs->started = false;

    if (!bs->instance)
    {
        Debug::LogError("ScriptSystem: no registered behavior named '%s' (entity %u). "
            "Is this script's .cpp compiled into this binary?",
            bs->behaviorName.c_str(), (unsigned)e);
    }
}

void ScriptSystem::OnEntityRemoved(EntityID e)
{
    BehaviorScript* bs = m_ecsRef->GetComponent<BehaviorScript>(e);
    if (bs && bs->instance && bs->started)
        bs->instance->OnDestroy();
}

void ScriptSystem::OnAllEntitiesRemoved()
{
    for (EntityID e : entities)
    {
        BehaviorScript* bs = m_ecsRef->GetComponent<BehaviorScript>(e);
        if (bs && bs->instance && bs->started)
            bs->instance->OnDestroy();
    }
}