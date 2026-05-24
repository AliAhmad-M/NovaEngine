#pragma once

#include <Core/Utilities/Vector2.h>

/**
 * @brief Transform data in a 2D plane
 */
struct Transform2D
{
    Vector2 position; 
    float rotation;
    Vector2 scale;

    Transform2D(const Vector2& position=Vector2::ZERO, float rotation=0.0f, const Vector2& scale=Vector2::ONE)
        : position(position), rotation(rotation), scale(scale) { }
};