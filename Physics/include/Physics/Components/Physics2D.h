#pragma once
#include <cstdio>
#include <Core/Utilities/Vector2.h>

/**
 * @brief Component representing a 2D physics rigid body.
 */
struct Physics2D
{
    Vector2 velocity;
    Vector2 acceleration;
    bool isStationary;

    /**
     * @brief Constructs a Physics2D component with the given mass and stationary state.
     * @param mass The mass of the body. Must be positive and non-zero.
     * @param isStationary If true, the body will not be affected by physics.
     */
    Physics2D(float mass = 1.0f, bool isStationary = false)
        : velocity(Vector2::ZERO), acceleration(Vector2::ZERO), isStationary(isStationary)
    {
        SetMass(mass);
    }

    /**
     * @brief Applies a force to the body by computing acceleration from F = ma.
     * @param amount The force vector to apply.
     */
    void ApplyForce(const Vector2& amount)
    {
        acceleration = Vector2::Divide(amount, m_mass);
    }

    /**
     * @brief Returns the mass of the body.
     * @return The mass as a float.
     */
    float GetMass()
    {
        return m_mass;
    }

    /**
     * @brief Sets the mass of the body. Warns and corrects invalid values.
     * @param mass The desired mass. Must be positive and non-zero.
     */
    void SetMass(float mass)
    {
        if (mass == 0.0f)
        {
            printf("[WARN]: Mass cannot be 0.0f, setting it to 1.0f\n");
        }
        else if (mass < 0.0f)
        {
            printf("[WARN]: Mass must be positive, inverting the sign\n");
            m_mass = -mass;
        }
        else
        {
            m_mass = mass;
        }
    }

private:
    float m_mass;
};