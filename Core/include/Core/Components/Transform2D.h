#pragma once

#include <Core/Utilities/Vector2.h>

/**
 * @brief Transform data in a 2D plane
 */
struct Transform2D
{
    Vector2 position; 
    float rotation; 
    float scaleX;
    float scaleY;

    Transform2D(const Vector2& position = Vector2(0, 0), float rotation = 0.0f, float scaleX = 1.0f, float scaleY = 1.0f)
        : position(position), rotation(rotation), scaleX(scaleX), scaleY(scaleY) { }
};