#pragma once
#include "Types.h"
#include <vector>
/**
 * Interface for type-erased component storage
 * Allows polymorphic handling of different component types
 */
class ISparseSet {
public:
    virtual ~ISparseSet() = default;
    virtual void OnEntityDestroyed(EntityID e) = 0;
    virtual void OnAllEntitiesDestroyed() = 0;
};

/**
 * Sparse set container for ECS components
 * Provides O(1) access, addition, and removal
 *
 * @tparam T Component type
 */
template <typename T>
class SparseSet : public ISparseSet {
public:
    /**
     * Reserves memory for components
     * Call once after creation
     */
    void Init() {
        m_dense.reserve(MAX_COMPONENTS);
        m_denseToEntity.reserve(MAX_COMPONENTS);
    }

    /**
     * Adds or updates a component for an entity
     * @param e Target entity
     * @param component Component data to store
     */
    void AddData(EntityID e, T component) {
        // Overwrite existing data
        if (HasData(e)) {
            *GetData(e) = std::move(component);
            return;
        }

        // Add component to the dense array
        setDenseIndex(e, (uint32_t)m_dense.size());
        m_dense.push_back(std::move(component));
        m_denseToEntity.push_back(e);
    }

    /**
     * Checks if entity has this component type
     * @param e Target entity
     * @return True if component exists
     */
    bool HasData(EntityID e) {
        return getDenseIndex(e) != NULL_INDEX;
    }

    /**
     * Gets pointer to entity's component
     * @param e Target entity
     * @return Pointer to component, nullptr if not found
     */
    T* GetData(EntityID e) {
        uint32_t index = getDenseIndex(e);
        if (index == NULL_INDEX) {
            return nullptr;
        }
        return &m_dense[index];
    }

    /**
     * Removes component from entity
     * Uses swap-and-pop for O(1) removal
     * @param e Target entity
     */
    void RemoveData(EntityID e) {
        uint32_t index = getDenseIndex(e);
        if (index == NULL_INDEX) {
            fprintf(stderr, "[WARN]: Entity %u does not have component of type: %s\n", e, typeid(T).name());
            return;
        }

        // Entity of the last dense element
        EntityID lastEntity = m_denseToEntity.back();

        // Swap the element to be deleted with the last element
        std::swap(m_dense[index], m_dense.back());
        std::swap(m_denseToEntity[index], m_denseToEntity.back());

        // Update the index for swapped entity
        setDenseIndex(lastEntity, index);
        setDenseIndex(e, NULL_INDEX);

        // Remove the last component and its corresponding entity ID
        m_dense.pop_back();
        m_denseToEntity.pop_back();
    }

    /**
     * Removes all components
     * Called when all entities are destroyed
     */
    void OnAllEntitiesDestroyed() override {
        m_dense.clear();
        m_denseToEntity.clear();
        m_sparsePages.clear();
    }

    /**
     * Removes component for a specific entity
     * Called when an entity is destroyed
     * @param e Destroyed entity
     */
    void OnEntityDestroyed(EntityID e) override {
        if (HasData(e)) {
            RemoveData(e);
        }
    }

private:
    /**
     * Stores the dense index for an entity in paged sparse array
     * @param e Target entity
     * @param index Index in dense arrays (or NULL_INDEX)
     */
    void setDenseIndex(EntityID e, uint32_t index) {
        uint32_t page = e / MAX_PAGE_SIZE;
        uint32_t pageIndex = e % MAX_PAGE_SIZE;

        // Create a new page if needed
        if (page >= m_sparsePages.size()) {
            m_sparsePages.resize(page + 1);
            m_sparsePages[page].resize(MAX_PAGE_SIZE, NULL_INDEX);
        }

        m_sparsePages[page][pageIndex] = index;
    }

    /**
     * Retrieves the dense index for an entity
     * @param e Target entity
     * @return Index in dense arrays, or NULL_INDEX if not found
     */
    uint32_t getDenseIndex(EntityID e) {
        uint32_t page = e / MAX_PAGE_SIZE;
        uint32_t pageIndex = e % MAX_PAGE_SIZE;

        if (page < m_sparsePages.size()) {
            return m_sparsePages[page][pageIndex];
        }
        return NULL_INDEX;
    }

    std::vector<std::vector<uint32_t>> m_sparsePages; // Paged sparse array mapping entity -> dense index
    std::vector<T> m_dense;                           // Packed component data
    std::vector<EntityID> m_denseToEntity;            // Packed entity IDs (parallel to m_dense)
};