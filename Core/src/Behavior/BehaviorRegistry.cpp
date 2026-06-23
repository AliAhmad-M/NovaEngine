#include <Core/Behavior/BehaviorRegistry.h>
#include <Core/Debug.h>

using namespace Nova;

BehaviorRegistry& BehaviorRegistry::Get()
{
    static BehaviorRegistry instance;
    return instance;
}

void BehaviorRegistry::Register(const std::string& name, Factory factory)
{
    if (m_factories.count(name))
    {
        Debug::LogWarn("Behavior '%s' already registered", name.c_str());
        return;
    }
    m_names.push_back(name);
    m_factories[name] = std::move(factory);
    Debug::LogInfo("Registered: %s", name.c_str());
}

std::unique_ptr<Behavior> BehaviorRegistry::Create(const std::string& name) const
{
    auto it = m_factories.find(name);
    if (it != m_factories.end())
        return it->second();

    Debug::LogError("Unknown behavior name: %s", name.c_str());
    return nullptr;
}

const std::vector<std::string>& BehaviorRegistry::GetNames() const
{
    return m_names;
}