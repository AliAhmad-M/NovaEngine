#pragma once
#include <unordered_map>
#include <typeindex>
#include <any>
#include <string>
#include <stdexcept>

class EngineContext
{
public:
    EngineContext() = default;
    ~EngineContext() = default;

    EngineContext(const EngineContext&) = delete;
    EngineContext& operator=(const EngineContext&) = delete;

    // Check if context has subsystem
	template <typename T>
    bool Has() const 
    {
        return m_services.find(typeid(T)) != m_services.end();
    }

    // Retrieve a subsystem pointer
    template<typename T>
    T* Get() const
    {
        auto it = m_services.find(typeid(T));
        if (it != m_services.end())
            return std::any_cast<T*>(it->second);
        return nullptr;
    }

    // Register a subsystem pointer
    template<typename T>
    void Register(T* service)
    {
        if (!service) return;
        m_services[typeid(T)] = service;
    }

    // Unregister a subsystem pointer
    template<typename T>
    void Unregister()
    {
        auto it = m_services.find(typeid(T));
        if (it != m_services.end())
            m_services.erase(it);
    }

private:
    std::unordered_map<std::type_index, std::any> m_services;
};