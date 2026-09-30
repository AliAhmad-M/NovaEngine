#include "MainShipMovement.hpp"
#include <Core/Behavior/BehaviorRegistry.h>

#include <algorithm>
#include <cmath>

using namespace Nova;

namespace
{
    // Steps current toward target by at most maxDelta
    float moveTowards(float current, float target, float maxDelta)
    {
        if (std::abs(target - current) <= maxDelta)
            return target;
        return current + (target > current ? maxDelta : -maxDelta);
    }
}

void MainShipMovement::OnStart()
{
    m_InputMapRef = Nova::Services::Fetch<InputMap>();
    m_transform = GetComponent<Transform2D>();
}

void MainShipMovement::OnUpdate(float dt)
{
    if (!m_InputMapRef || !m_transform) return;

    Vector2 rawInput = m_InputMapRef->GetRawInputVector(
        GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN
    );

    bool hasInput = (rawInput.x != 0.0f || rawInput.y != 0.0f);

    // Desired movement direction
    Vector2 desiredDir = hasInput ? rawInput.Normalize() : Vector2{ 0.0f, 0.0f };
    Vector2 targetVelocity = desiredDir * maxSpeed;

    // Rate of acceleration/decelaration
    float rate = hasInput ? acceleration : deceleration;
    float maxDelta = rate * dt;

    m_velocity.x = moveTowards(m_velocity.x, targetVelocity.x, maxDelta);
    m_velocity.y = moveTowards(m_velocity.y, targetVelocity.y, maxDelta);

    m_transform->position += m_velocity * dt;

    // Clamp to playfield
    if (m_transform->position.x < kMinX) { m_transform->position.x = kMinX; m_velocity.x = 0.0f; }
    if (m_transform->position.x > kMaxX) { m_transform->position.x = kMaxX; m_velocity.x = 0.0f; }
    if (m_transform->position.y < kMinY) { m_transform->position.y = kMinY; m_velocity.y = 0.0f; }
    if (m_transform->position.y > kMaxY) { m_transform->position.y = kMaxY; m_velocity.y = 0.0f; }
}

REGISTER_BEHAVIOR(MainShipMovement)