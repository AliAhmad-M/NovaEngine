#pragma once
#include <array>
#include "Types.h"

/**
 * @brief Base interface for all systems with a set of entities
 */ 
struct ISystem {
	std::array<bool, MAX_ENTITIES> entityRecord;

	virtual void OnEntityAdded(EntityID e) = 0;
	virtual void OnEntityRemoved(EntityID e) = 0;
	virtual void OnAllEntitiesRemoved() = 0;
};
