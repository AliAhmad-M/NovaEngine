#pragma once
#include <cstdio>
#include <algorithm>
#include <Core/Utilities/Vector2.h>

/**
 * @brief Component representing a 2D physics body
 */
struct Physics2D
{
    // The current rate of change of position
    Vector2 linearVelocity;

    // The current rate of change of velocity
    Vector2 acceleration;

    // Accumulated net force
    Vector2 force;

    // Exponential linear velocity decay factor
    float linearDamping;

    // Coefficient of bounciness
    float restitution;

    // Flag determining if body is stationary
    bool isStatic;

    /**
     * @brief Constructs a Physics2D component with the given mass and stationary state
     * @param mass The mass of the body. Must be positive and non-zero
     * @param isStatic If true, the body will act as an unmovable object with infinite mass
     * @param damping Rate of exponential velocity decay over time
     * @param bounciness Restitution coefficient (0.0 to 1.0)
     */
    Physics2D(float mass = 1.0f, bool isStatic = false, float damping = 0.0f, float bounciness = 0.2f)
        : linearVelocity(Vector2::ZERO), acceleration(Vector2::ZERO), force(Vector2::ZERO),
        linearDamping(damping), restitution(bounciness), isStatic(isStatic)
    {
        SetMass(mass);
    }

    /**
     * @brief Accumulates an external force vector
     * @param amount The force vector to add to the total accumulated net force
     */
    void ApplyForce(const Vector2& amount)
    {
        force += amount;
    }

    /**
     * @brief Retrieves the current mass of the physics body
     * @return The scalar mass value
     */
    float GetMass() const
    {
        return m_mass;
    }

    /**
     * @brief Retrieves the inverse mass (1 / m) of the physics body
     * @return The inverse mass value.
     */
    float GetInverseMass() const
    {
        return m_invMass;
    }

    /**
     * @brief Updates the body's mass
     * @param mass The new scalar mass value. Must be strictly positive and non-zero
     */
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
        m_invMass = isStatic ? 0.0f : (1.0f / m_mass);
    }

private:
    float m_mass;
    float m_invMass;
};