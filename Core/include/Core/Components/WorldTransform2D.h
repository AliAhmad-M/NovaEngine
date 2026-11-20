#pragma once
#include <Core/Utilities/Vector2.h>

namespace Nova 
{
    /**
     @brief Computed as the local transform + root transform
     */
    struct WorldTransform2D
    {
        Vector2 position = Vector2::ZERO;
        float   rotation = 0.0f;
        Vector2 scale = Vector2::ONE;
    };
}