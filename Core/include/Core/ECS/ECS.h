#pragma once
#include <Core/EngineContext.h>
#include "Types.h"
#include "SparseSet.h"
#include "EntityManager.h"
#include "SystemManager.h"

/**
 * @brief Central ECS coordinator
 * @brief Manages entities, components, and systems
 */
class ECS 
{
public:
    /**
     * @brief Initializes ECS managers
     * @brief Call once after creation
     */
    void Init() 
    {
        printf("[INFO]: Initializing ECS...\n");

        m_entityManager = std::make_unique<EntityManager>();
        m_systemManager = std::make_unique<SystemManager>();
        m_entityManager->Init();

        printf("[INFO]: ECS initialized\n");
    }

    /**
     * @brief Creates a new entity
     * @return New entity ID
     */
    EntityID CreateEntity() 
    {
        return m_entityManager->CreateEntity();
    }

    /**
     * @brief Deletes an entity and all its components
     * @param entity Entity to delete
     */
    void DeleteEntity(EntityID entity) {
        m_entityManager->DeleteEntity(entity);
        m_systemManager->OnEntityDestroyed(entity);

        for (auto& pair : m_sparseSets) {
            pair.second->OnEntityDestroyed(entity);
        }
    }

    /**
     * @brief Deletes all entities and components
     */
    void DeleteAllEntities() {
        m_entityManager->ClearEntites();
        m_systemManager->OnAllEntitesDestroyed();

        for (auto& pair : m_sparseSets) {
            pair.second->OnAllEntitiesDestroyed();
        }
    }

    /**
     * @brief Gets current active entity count
     * @return Number of active entities
     */
    size_t GetEntityCount() 
    {
        return m_entityManager->GetEntityCount();
    }

    /**
     * @brief Registers a component type with the ECS
     * @tparam T Component type
     */
    template <typename T>
    void RegisterComponent() 
    {
        const std::type_index typeIndex(typeid(T));
        const char* componentType = typeIndex.name();
        printf("[INFO]: Registering component: %s...\n", typeIndex.name());

        // Check if component already registered
        if (m_componentRegistry.count(typeIndex)) 
        {
            printf("[WARN]: Component '%s' already registered, skipping\n", typeIndex.name());
            return;
        }

        auto sparseSet = std::make_unique<SparseSet<T>>();
        sparseSet->Init();

        m_sparseSets[typeIndex] = std::move(sparseSet);
        m_componentRegistry[typeIndex] = m_nextComponentId;
        m_nextComponentId++;

        printf("[INFO]: Registered component: %s successfully\n", typeIndex.name());
    }

    /**
     * @brief Adds a component to an entity
     * @tparam T Component type
     * @param entity Target entity
     * @param component Component data
     */
    template <typename T>
    void AddComponent(EntityID entity, T component) 
    {
        if (!m_entityManager->IsActive(entity)) 
        {
            fprintf(stderr, "[WARN]: Cannot add component on non existing entity\n");
            return;
        }

        getComponentSparseSet<T>()->AddData(entity, std::move(component));
        updateEntitySignature<T>(entity, true);
    }

    /**
     * @brief Removes a component from an entity
     * @tparam T Component type
     * @param entity Target entity
     */
    template <typename T>
    void RemoveComponent(EntityID entity) 
    {
        if (!m_entityManager->IsActive(entity)) 
        {
            fprintf(stderr, "[WARN]: Cannot remove component on non existing entity\n");
            return;
        }

        getComponentSparseSet<T>()->RemoveData(entity);
        updateEntitySignature<T>(entity, false);
    }

    /**
     * @brief Checks if entity has a component
     * @tparam T Component type
     * @param entity Target entity
     * @return True if component exists
     */
    template <typename T>
    bool HasComponent(EntityID entity) 
    {
        return getComponentSparseSet<T>()->HasData(entity);
    }

    /**
     * @brief Gets pointer to entity's component
     * @tparam T Component type
     * @param entity Target entity
     * @return Pointer to component, nullptr if not found
     */
    template <typename T>
    T* GetComponent(EntityID entity) 
    {
        return getComponentSparseSet<T>()->GetData(entity);
    }

    /**
     * @brief Gets the unique ID for a component type
     * @tparam T Component type
     * @return Component ID
     */
    template <typename T>
    ComponentID GetComponentID() 
    {
        std::type_index typeIndex = typeid(T);
        auto it = m_componentRegistry.find(typeIndex);
        if (it != m_componentRegistry.end()) {
            return it->second;
        }
        fprintf(stderr, "[ERROR]: Cannot get ID on unregistered component");
        exit(1);
    }

    /**
     * @brief Registers a system
     * @tparam T System type
     * @tparam Args Additional constructor argument types
     * @param args Additional constructor arguments (e.g. ECS*, Camera*, Renderer*)
     * @return Shared pointer to registered system
     */
    template <typename T>
    std::shared_ptr<T> RegisterSystem(EngineContext* ctx) 
    {
        return m_systemManager->RegisterSystem<T>(ctx);
    }

    /**
     * @brief Retrieves a registered system
     * @tparam T System type
     * @return Shared pointer to system
     */
    template <typename T>
    std::shared_ptr<T> GetSystem() 
    {
        return std::static_pointer_cast<T>(m_systemManager->GetSystem<T>());
    }

    /**
     * @brief Adds a component type to a system's signature
     * @tparam T Component type
     * @tparam U System type
     */
    template <typename T, typename U>
    void AddComponentToSystem() 
    {
        Signature signature = m_systemManager->GetSignature<U>();
        signature.set(GetComponentID<T>(), true);
        m_systemManager->SetSignature<U>(signature);
    }

private:
    /**
     * @brief Retrieves the sparse set for a component type
     * @tparam T Component type
     * @return Pointer to sparse set
     */
    template <typename T>
    SparseSet<T>* getComponentSparseSet() 
    {
        const std::type_index typeIndex(typeid(T));
        auto it = m_sparseSets.find(typeIndex);
        if (it != m_sparseSets.end()) {
            return static_cast<SparseSet<T>*>(it->second.get());
        }

        fprintf(stderr, "[ERROR]: Component map for type not registered");
        exit(1);
    }

    /**
     * @brief Updates an entity's signature and notifies systems
     * @tparam T Component type being added/removed
     * @param entity Target entity
     * @param value True if adding, false if removing
     */
    template <typename T>
    void updateEntitySignature(EntityID entity, bool value) 
    {
        Signature signature = m_entityManager->GetSignature(entity);
        signature.set(GetComponentID<T>(), value);
        m_entityManager->SetSignature(entity, signature);
        m_systemManager->OnEntitySignatureChanged(entity, signature);
    }

    std::unique_ptr<EntityManager> m_entityManager;
    std::unique_ptr<SystemManager> m_systemManager;

    std::unordered_map<std::type_index, ComponentID> m_componentRegistry;
    std::unordered_map<std::type_index, std::unique_ptr<ISparseSet>> m_sparseSets;
    ComponentID m_nextComponentId = 0;
};