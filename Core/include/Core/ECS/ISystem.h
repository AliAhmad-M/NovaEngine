#pragma once
#include <unordered_set>
#include "Types.h"

/**
 * @brief Base interface for all systems with a set of entities
 */ 
struct ISystem {
	std::unordered_set<EntityID> entities;

	virtual void OnEntityAdded(EntityID e) = 0;
	virtual void OnEntityRemoved(EntityID e) = 0;
	virtual void OnAllEntitiesRemoved() = 0;
};
