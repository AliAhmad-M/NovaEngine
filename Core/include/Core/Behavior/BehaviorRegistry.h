#pragma once
#include <Core/Behavior/Behavior.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Nova
{
    class BehaviorRegistry
    {
    public:
        using Factory = std::function<std::unique_ptr<Behavior>()>;

        static BehaviorRegistry& Get();

        void Register(const std::string& name, Factory factory);
        std::unique_ptr<Behavior> Create(const std::string& name) const;
        const std::vector<std::string>& GetNames() const;

    private:
        BehaviorRegistry() = default;

        std::vector<std::string>                 m_names;
        std::unordered_map<std::string, Factory> m_factories;
    };
}

#define REGISTER_BEHAVIOR(ClassName)                                              \
    namespace {                                                                   \
        struct ClassName##_Registrar {                                            \
            ClassName##_Registrar() {                                             \
                Nova::BehaviorRegistry::Get().Register(                           \
                    #ClassName,                                                   \
                    []() -> std::unique_ptr<Nova::Behavior> {                     \
                        return std::make_unique<ClassName>();                     \
                    });                                                           \
            }                                                                     \
        } ClassName##_registrar_instance;                                         \
    }