#pragma once

#define NOMINMAX
#include <Core/Utilities/Vector2.h>
#include <cmath>
#include <algorithm>

namespace Nova 
{
    /**
 * @brief Transform data in a 2D plane
 */
    struct Transform2D
    {
        Vector2 position;
        float rotation;
        Vector2 scale;

        Transform2D(const Vector2& position = Vector2::ZERO, float rotation = 0.0f, const Vector2& scale = Vector2::ONE)
            : position(position), rotation(rotation)
        {
            this->scale.x = scale.x < 0.001f ? 0.001f : scale.x;
            this->scale.y = scale.y < 0.001f ? 0.001f : scale.y;
        }
    };
}

