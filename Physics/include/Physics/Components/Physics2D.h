#pragma once
#include <cstdio>
#include <algorithm>
#include <Core/Utilities/Vector2.h>

/**
 * @brief Component representing a 2D physics body
 */
struct Physics2D
{
    Vector2 linearVelocity = Vector2::ZERO;
    float angularVelocity = 0.0f;

    Vector2 acceleration = Vector2::ZERO;
    Vector2 force = Vector2::ZERO;

    bool lockPosition;
    bool lockRotation;
    /**
     * @brief Constructs a Physics2D component with the given mass and stationary state
     * @param mass The mass of the body. Must be positive and non-zero
     * @param isStatic If true, the body will act as an unmovable object with infinite mass
     * @param damping Rate of exponential velocity decay over time
     * @param bounciness Restitution coefficient (0.0 to 1.0)
     */
    Physics2D(float mass = 1.0f, float restitution = 0.2f, float staticFriction = 0.5f, float dynamicFriction = 0.3f,
        float linearDamping = 0.0f, float angularDamping = 0.0f, bool lockPosition = false, bool lockRotation = false)
        : m_restitution(restitution), lockPosition(lockPosition), lockRotation(lockRotation)
    {
        SetMass(mass);
        SetRestitution(restitution);
        SetStaticFriction(staticFriction);
        SetDynamicFriction(dynamicFriction);
        SetLinearDamping(linearDamping);
        SetAngularDamping(angularDamping);
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
        m_invMass = lockPosition ? 0.0f : (1.0f / m_mass);
    }

/**
     * @brief Retrieves the current coefficient of restitution
     * @return The scalar restitution value (usually between 0.0 and 1.0)
     */
    float GetRestitution() const 
    {
        return m_restitution;
    }

    /**
     * @brief Updates the coefficient of restitution
     * @param restitution The new restitution value. Will be clamped between 0.0f and 1.0f
     */
    void SetRestitution(float restitution) 
    {
        m_restitution = std::clamp(restitution, 0.0f, 1.0f);
    }

    /**
     * @brief Retrieves the current static friction coefficient
     * @return The static friction coefficient
     */
    float GetStaticFriction() const 
    {
        return m_staticFriction;
    }

    /**
     * @brief Updates the static friction coefficient
     * @param staticFriction The new static friction value. Must be non-negative
     */
    void SetStaticFriction(float staticFriction) 
    {
        m_staticFriction = std::max(0.0f, staticFriction);
    }

    /**
     * @brief Retrieves the current dynamic friction coefficient
     * @return The dynamic friction coefficient
     */
    float GetDynamicFriction() const 
    {
        return m_dynamicFriction;
    }

    /**
     * @brief Updates the dynamic friction coefficient
     * @param dynamicFriction The new dynamic friction value. Must be non-negative
     */
    void SetDynamicFriction(float dynamicFriction) 
    {
        m_dynamicFriction = std::max(0.0f, dynamicFriction);
    }

    /**
     * @brief Retrieves the current linear damping coefficient
     * @return The linear damping coefficient
     */
    float GetLinearDamping() const 
    {
        return m_linearDamping;
    }

    /**
     * @brief Updates the linear damping coefficient
     * @param linearDamping The new linear damping value. Must be non-negative
     */
    void SetLinearDamping(float linearDamping) 
    {
        m_linearDamping = std::max(0.0f, linearDamping);
    }

    /**
     * @brief Retrieves the current angular damping coefficient
     * @return The angular damping coefficient
     */
    float GetAngularDamping() const 
    {
        return m_angularDamping;
    }

    /**
     * @brief Updates the angular damping coefficient
     * @param angularDamping The new angular damping value. Must be non-negative
     */
    void SetAngularDamping(float angularDamping)
    {
        m_angularDamping = std::max(0.0f, angularDamping);
    }

private:
    float m_mass;
    float m_invMass;

    float m_restitution;
    float m_staticFriction;
    float m_dynamicFriction;

    float m_linearDamping;
    float m_angularDamping;
};