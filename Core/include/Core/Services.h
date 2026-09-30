#pragma once
#include "Debug.h"
#include <unordered_map>
#include <typeindex>
#include <any>
#include <cassert>

namespace Nova 
{
    class Services
    {
    public:
        static void Init()
        {
            if (s_instanceRef)
                Debug::LogWarn("Services already initialized");

            s_instanceRef = new Services();
        }

        static void Shutdown()
        {
            delete s_instanceRef;
            s_instanceRef = nullptr;
        }

        static Services& Get()
        {
            if (!s_instanceRef)
            {
                static Services fallback;
                return fallback;
            }

            return *s_instanceRef;
        }

        template<typename T>
        static bool Has()
        {
            return Get().m_services.find(typeid(T)) != Get().m_services.end();
        }

        template<typename T>
        static T* Fetch()
        {
            auto it = Get().m_services.find(typeid(T));
            if (it != Get().m_services.end())
                return std::any_cast<T*>(it->second);

            Debug::LogError("Service '%s' has not been registered yet", typeid(T).name());
            return nullptr;
        }

        template<typename T>
        static void Register(T* service)
        {
            if (!service) return;
            Get().m_services[typeid(T)] = service;
        }

        template<typename T>
        static void Unregister()
        {
            auto& m = Get().m_services;
            auto it = m.find(typeid(T));
            if (it != m.end()) m.erase(it);
        }

    private:
        Services() = default;
        ~Services() = default;

        Services(const Services&) = delete;
        Services& operator=(const Services&) = delete;

        std::unordered_map<std::type_index, std::any> m_services;
        inline static Services* s_instanceRef = nullptr;
    };
}