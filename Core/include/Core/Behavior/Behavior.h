#pragma once
#include <Core/ECS/ECS.h>
#include "Inspectables.h"

namespace Nova
{
    class Behavior
    {
    public:
        virtual ~Behavior() = default;

        virtual void OnStart() {}
        virtual void OnUpdate(float dt) {}
        virtual void OnDestroy() {}
        virtual void OnInspect() {}
        virtual void ReflectFields(Inspectables& visitor) {}

        virtual const char* GetName() const = 0;

    protected:
        template<typename T>
        T* GetComponent() { return m_ecsRef->GetComponent<T>(m_entity); }

        template<typename T>
        bool HasComponent() { return m_ecsRef->HasComponent<T>(m_entity); }

        template<typename T, typename... Args>
        void AddComponent(Args&&... args)
        {
            m_ecsRef->AddComponent<T>(m_entity, std::forward<Args>(args)...);
        }

        EntityID GetEntity() const { return m_entity; }

    private:
        ECS* m_ecsRef = nullptr;
        EntityID m_entity = static_cast<EntityID>(-1);

        friend class ScriptSystem;
    };
}