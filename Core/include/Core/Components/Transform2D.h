#pragma once

#include <Core/Utilities/Vector2.h>
#include <Core/Behavior/Inspectables.h>
#include <cmath>

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
            this->scale.x = std::max(0.001f, scale.x);
            this->scale.y = std::max(0.001f, scale.y);
        }

        void ReflectFields(Inspectables& visitor)
        {
            visitor.Vec2("Position", &position);
            visitor.Float("Rotation", &rotation);
            visitor.Vec2("Scale", &scale);
        }
    };
}

