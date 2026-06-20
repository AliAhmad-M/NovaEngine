#pragma once
#include "Types.h"
#include <queue>
#include <vector>

namespace Nova 
{
    /**
     * @brief Manages entity creation, deletion, and signatures
     * @brief Uses dynamic growth to avoid preallocating MAX_ENTITIES
     */
    class EntityManager
    {
    public:
        /**
         * @brief Initializes entity manager with initial capacity
         */
        void Init();

        /**
         * @brief Checks if an entity ID is currently active
         * @param entity Entity ID to check
         * @return True if entity exists and is active
         */
        bool IsActive(EntityID entity);

        /**
         * @brief Creates a new entity ID
         * @return New unique entity ID
         * @throws std::runtime_error if unable to create entity
         */
        EntityID CreateEntity();

        /**
         * @brief Deletes an entity, making its ID available for reuse
         * @param entity Entity ID to delete
         */
        void DeleteEntity(EntityID entity);

        /**
         * @brief Deletes all entities and resets manager to initial state
         */
        void ClearEntites();

        /**
         * @brief Returns current number of active entities
         * @return Active entity count
         */
        uint32_t GetEntityCount();

        /**
         * @brief Sets the component signature for an entity
         * @param entity Target entity
         * @param signature Bitset of component types
         */
        void SetSignature(EntityID entity, Signature signature);

        /**
         * @brief Gets the component signature for an entity
         * @param entity Target entity
         * @return Signature bitset
         * @throws std::runtime_error if entity doesn't exist
         */
        Signature GetSignature(EntityID entity);

    private:
        void grow();

        std::deque<EntityID> m_availableEntities{};
        std::vector<bool> m_entityStatus;
        std::vector<Signature> m_signatures;

        uint32_t m_entityCount = 0;
        uint32_t m_capacity = 0;
    };
}

