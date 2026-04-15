#include <Core/ECS/EntityManager.h>
#include <numeric>
#include <cstdio>

void EntityManager::Init() {
    m_capacity = MAX_PAGE_SIZE;
    m_entityStatus.resize(m_capacity, false);
    m_signatures.resize(m_capacity);

    // Initialize available IDs
    std::vector<EntityID> temp(m_capacity);
    std::iota(temp.begin(), temp.end(), 0);
    m_availableEntities.resize(m_capacity);
    std::iota(m_availableEntities.begin(), m_availableEntities.end(), 0);
}

void EntityManager::grow() {
    uint32_t oldCapacity = m_capacity;
    m_capacity += MAX_PAGE_SIZE;

    // Check against the global limit
    if (m_capacity > MAX_ENTITIES) {
        m_capacity = MAX_ENTITIES;
    }

    // Resize data vectors to new capacity
    m_entityStatus.resize(m_capacity, false);
    m_signatures.resize(m_capacity);

    // Add new IDs the available queue
    for (EntityID i = oldCapacity; i < m_capacity; ++i) {
        m_availableEntities.push_back(i);
    }
}

bool EntityManager::IsActive(EntityID entity) {
    return entity < m_capacity && m_entityStatus[entity];
}

EntityID EntityManager::CreateEntity() {
    // Add more IDs if all are used up
    if (m_availableEntities.empty()) {
        grow();
    }

    // Get the next available entity ID
    if (!m_availableEntities.empty()) {
        EntityID entity = m_availableEntities.front();
        m_availableEntities.pop_front();
        m_entityStatus[entity] = true;
        m_entityCount++;
        return entity;
    }

    fprintf(stderr, "[ERROR]: Failed to create entity");
    exit(1);
}


void EntityManager::DeleteEntity(EntityID entity)
{
    // Delete entity if is active
    if (IsActive(entity)) {
        m_availableEntities.push_back(entity);
        m_entityStatus[entity] = false;
        m_entityCount--;
        m_signatures[entity].reset();
    }
    else {
        fprintf(stderr, "[ERROR]: Cannot delete non existent entity\n");
    }
}

void EntityManager::ClearEntites() {
    // Clear all entities
    m_entityStatus.assign(m_capacity, false);
    m_signatures.clear();
    m_signatures.resize(m_capacity);

    // Reset available IDs
    m_availableEntities.resize(m_capacity);
    std::iota(m_availableEntities.begin(), m_availableEntities.end(), 0);

    m_entityCount = 0;
}

uint32_t EntityManager::GetEntityCount() {
    return m_entityCount;
}

void EntityManager::SetSignature(EntityID entity, Signature signature) {
    // Assign the component signature
    if (IsActive(entity)) {
        m_signatures[entity] = signature;
    }
    else {
        fprintf(stderr, "[ERROR]: Cannot set signature on non existent entity\n");
    }
}

Signature EntityManager::GetSignature(EntityID entity) {
    // Get component signature of entity
    if (IsActive(entity)) {
        return m_signatures[entity];
    }

    fprintf(stderr, "[ERROR]: Cannot get signature on non existent entity\n");
    exit(1);
}