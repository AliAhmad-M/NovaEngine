#pragma once
#include <limits>
#include <cstdint>

constexpr std::uint32_t MAX_ENTITIES = 131072;
constexpr std::uint8_t MAX_COMPONENTS = 32;

constexpr uint32_t INVALID_ENTITY = MAX_ENTITIES;
constexpr size_t MAX_PAGE_SIZE = 1024;