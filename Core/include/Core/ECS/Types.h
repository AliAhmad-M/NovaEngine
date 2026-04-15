#pragma once
#include <cstdint>
#include <bitset>
#include "Constants.h"

/**
 * Unique identifier for entity
 */
using EntityID = uint32_t;

/**
 * Unique identifier for component
 */
using ComponentID = uint8_t;

/**
 * Tells us which components an entity has
 */
using Signature = std::bitset<MAX_COMPONENTS>;