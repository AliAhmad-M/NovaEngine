#pragma once
#include <cstdio>
#include <algorithm>
#include <Core/Utilities/Vector2.h>

/**
 * @brief Component representing a 2D physics rigid body updated with modern integration features.
 */
struct Physics2D
{
    Vector2 velocity;
    Vector2 acceleration;
    Vector2 force;
    float linearDamping;
    float restitution;
    bool isStationary;

    /**
     * @brief Constructs a Physics2D component with the given mass and stationary state.
     * @param mass The mass of the body. Must be positive and non-zero.
     * @param isStationary If true, the body will not be affected by physics.
     * @param damping Rate of exponential velocity decay over time.
     * @param bounciness Restitution coefficient (0.0 to 1.0).
     */
    Physics2D(float mass = 1.0f, bool isStationary = false, float damping = 0.0f, float bounciness = 0.2f)
        : velocity(Vector2::ZERO), acceleration(Vector2::ZERO), force(Vector2::ZERO),
        linearDamping(damping), restitution(bounciness), isStationary(isStationary)
    {
        SetMass(mass);
    }

    /**
     * @brief Accumulates a force to be solved during the integration pass.
     * @param amount The force vector to apply.
     */
    void ApplyForce(const Vector2& amount)
    {
        force += amount;
    }

    float GetMass() const { return m_mass; }
    float GetInverseMass() const { return m_invMass; }

    void SetMass(float mass)
    {
        if (mass <= 0.0f)
        {
            printf("[WARN]: Mass must be positive and non-zero, setting it to 1.0f\n");
            m_mass = 1.0f;
        }
        else
        {
            m_mass = mass;
        }

        // Inverse mass (0.0f for static bodies to represent infinite mass)
        m_invMass = isStationary ? 0.0f : (1.0f / m_mass);
    }

private:
    float m_mass;
    float m_invMass;
};