#pragma once
#include <Core/EngineContext.h>
#include "ISystem.h"
#include <memory>
#include <unordered_map>
#include <typeindex>

/**
 * @brief Manages registration, storage, and retrieval of systems
 * @brief Systems are stored as shared pointers and keyed by type
 */
class SystemManager {
public:
    /**
     * @brief Registers a new system
     * @tparam T System type (must derive from ISystem)
     * @tparam Args Additional constructor argument types
     * @param args Additional constructor arguments (e.g. ECS*, Camera*, Renderer*)
     * @return Shared pointer to the newly created system
     */
    template <typename T>
    std::shared_ptr<T> RegisterSystem(EngineContext* ctx) {
        const std::type_index typeIndex(typeid(T));
        const char* systemType = typeIndex.name();
        
        printf("[INFO]: Registering system: %s...\n", systemType);

        // Check if system is already registered
        if (isRegistered(typeIndex)) {
            fprintf(stderr, "[WARN]: System '%s' is already registered, skipping\n", systemType);
            return nullptr;

        }

        auto system = std::make_shared<T>(ctx);
        m_systems[typeIndex] = system;
        printf("[INFO]: Registered system: %s successfully\n", systemType);
        return system;
    }

    /**
     * @brief Retrieves a registered system
     * @tparam T System type
     * @return Shared pointer to the system
     */
    template <typename T>
    std::shared_ptr<T> GetSystem() {
        const std::type_index typeIndex = typeid(T);
        if (isRegistered(typeIndex)) {
            return std::static_pointer_cast<T>(m_systems[typeIndex]);
        }
        fprintf(stderr, "[ERROR]: Cannot get unregistered system\n");
        exit(1);
    }

    /**
     * @brief Gets the component signature for a system
     * @tparam T System type
     * @return Signature bitset of components the system uses
     */
    template <typename T>
    Signature GetSignature() {
        const std::type_index typeIndex = typeid(T);
        if (isRegistered(typeIndex)) {
            return m_signatures[typeIndex];
        }
        fprintf(stderr, "[ERROR]: Cannot get signature on unregistered system\n");
        exit(1);
    }

    /**
     * @brief Sets the component signature for a system
     * @tparam T System type
     * @param signature Signature bitset of components the system uses
     */
    template <typename T>
    void SetSignature(Signature signature) {
        const std::type_index typeIndex = typeid(T);
        if (isRegistered(typeIndex)) {
            m_signatures[typeIndex] = signature;
        }
        else {
            fprintf(stderr, "[WARN]: Cannot set signature on unregistered system\n");
        }
    }

    /**
     * @brief Notifies all systems that an entity has been destroyed
     * @param entity ID of destroyed entity
     */
    void OnEntityDestroyed(EntityID entity);

    /**
     * @brief Notifies all systems that all entities have been cleared
     */
    void OnAllEntitesDestroyed();

    /**
     * @brief Notifies all systems that an entity's signature has changed
     * @param entity Entity whose signature changed
     * @param entitySignature New signature of the entity
     */
    void OnEntitySignatureChanged(EntityID entity, Signature entitySignature);

private:
    /**
     * @brief Checks if a system type is registered
     * @param typeIndex Type index of the system
     * @return True if system is registered
     */
    bool isRegistered(std::type_index typeIndex) {
        return m_systems.find(typeIndex) != m_systems.end();
    }

    std::unordered_map<std::type_index, Signature> m_signatures{};
    std::unordered_map<std::type_index, std::shared_ptr<ISystem>> m_systems{};
};