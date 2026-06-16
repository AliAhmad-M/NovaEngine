#pragma once
#include <box2d/box2d.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_world_callbacks.h>
#include <bitset>
#include <algorithm>
#include <Core/Utilities/Vector2.h>
#include <Core/Utilities/Shape2D.h>

static constexpr float PHYSICS_PPM = 64.0f;

struct RigidBody2D
{
public:
    b2Body* body = nullptr;
    b2Fixture* fixture = nullptr;
    bool isColliding = false;
    Vector2 normal = Vector2::ZERO;
    Vector2 centerOffset = Vector2::ZERO;

    /**
     * @brief Creates a circular physics body
     * @param centerOffset Shifting offset from the entity's position, in pixels
     * @param r The full radius of the circle, in pixels
     * @param isStatic If true, the body stays pinned in place and cannot move
     * @param isSensor If true, objects pass right through it but still trigger collision events
     * @param density Mass per unit volume in kilograms per meter squared (volume = area in 2D)
     * @param restitution How bouncy the body is, from 0 to 1
     * @param friction How slippery the surface is, from 0 to 1
     * @param lockRotation If true, prevents forces from making the body rotate
     */
    RigidBody2D(
        const Vector2& centerOffset,
        float r,
        bool isStatic = false,
        bool isSensor = false,
        float density = 1.0f,
        float restitution = 0.2f,
        float friction = 0.5f,
        bool lockRotation = false
    )
        : centerOffset(centerOffset)
        , m_shape(Shape2D::CIRCLE)
        , m_isStatic(isStatic)
        , m_isSensor(isSensor)
        , m_density(std::max(0.0f, density))
        , m_restitution(std::clamp(restitution, 0.0f, 1.0f))
        , m_friction(std::clamp(friction, 0.0f, 1.0f))
        , m_lockRotation(lockRotation)
    {
        m_circle.radius = std::max(0.001f, r);
        m_layer.set(0);
        m_mask.set(0);
    }

    /**
     * @brief Creates a box-shaped physics body
     * @param centerOffset Shifting offset from the entity's position, in pixels
     * @param w The total width of the box, in pixels
     * @param h The total height of the box, in pixels
     * @param isStatic If true, the body stays pinned in place and cannot move
     * @param isSensor If true, objects pass right through it but still trigger collision events
     * @param density Mass per unit volume in kilograms per meter squared (volume = area in 2D)
     * @param restitution How bouncy the body is, from 0 to 1
     * @param friction How slippery the surface is, from 0 to 1
     * @param lockRotation If true, prevents forces from making the body rotate
     */
    RigidBody2D(
        const Vector2& centerOffset,
        float w,
        float h,
        bool isStatic = false,
        bool isSensor = false,
        float density = 1.0f,
        float restitution = 0.2f,
        float friction = 0.5f,
        bool lockRotation = false
    )
        : centerOffset(centerOffset)
        , m_shape(Shape2D::BOX)
        , m_isStatic(isStatic)
        , m_isSensor(isSensor)
        , m_density(std::max(0.0f, density))
        , m_restitution(std::clamp(restitution, 0.0f, 1.0f))
        , m_friction(std::clamp(friction, 0.0f, 1.0f))
        , m_lockRotation(lockRotation)
    {
        m_box.halfW = std::max(0.001f, w * 0.5f);
        m_box.halfH = std::max(0.001f, h * 0.5f);
        m_layer.set(0);
        m_mask.set(0);
    }

    /**
     * @brief Gets the basic geometric classification type assigned to this body
     * @return Shape2D metadata flag
     */
    Shape2D GetShape() const { return m_shape; }

    /**
     * @brief Gets the stored radius
     * @return Radius size in pixels
     */
    float GetRadius() const { return m_shape == Shape2D::CIRCLE ? m_circle.radius : 0.0f; }

    /**
     * @brief Updates the radius dynamically
     * @param radius New radius in pixels
     * @param currentScale Optional multiplier
     */
    void SetRadius(float radius, float currentScale = 1.0f)
    {
        if (m_shape != Shape2D::CIRCLE) return;
        m_circle.radius = std::max(0.001f, radius);

        if (fixture)
        {
            auto* circleShape = dynamic_cast<b2CircleShape*>(fixture->GetShape());
            if (circleShape)
            {
                circleShape->m_radius = (m_circle.radius * currentScale) / PHYSICS_PPM;
                synchronizeMassData();
            }
        }
    }

    /**
     * @brief Gets half width span
     * @return Stored half width in pixels
     */
    float GetHalfWidth() const { return m_shape == Shape2D::BOX ? m_box.halfW : 0.0f; }

    /**
     * @brief Gets full width span
     * @return Width of the body in pixels
     */
    float GetFullWidth() const { return m_shape == Shape2D::BOX ? m_box.halfW * 2.0f : 0.0f; }

    /**
     * @brief Gets half height span
     * @return Stored half height in pixels
     */
    float GetHalfHeight() const { return m_shape == Shape2D::BOX ? m_box.halfH : 0.0f; }

    /**
     * @brief Gets full height span
     * @return Height of the body in pixels
     */
    float GetFullHeight() const { return m_shape == Shape2D::BOX ? m_box.halfH * 2.0f : 0.0f; }

    /**
     * @brief Updates the rectangle dimensions dynamically
     * @param w New width in pixels
     * @param h New height in pixels
     * @param currentScale Optional multiplier
     */
    void SetDimensions(float w, float h, const Vector2& currentScale = Vector2::ONE)
    {
        if (m_shape != Shape2D::BOX) return;
        m_box.halfW = std::max(0.001f, w * 0.5f);
        m_box.halfH = std::max(0.001f, h * 0.5f);

        if (fixture)
        {
            auto* polyShape = dynamic_cast<b2PolygonShape*>(fixture->GetShape());
            if (polyShape)
            {
                float scaledHalfW = (m_box.halfW * currentScale.x) / PHYSICS_PPM;
                float scaledHalfH = (m_box.halfH * currentScale.y) / PHYSICS_PPM;
                b2Vec2 scaledOffset = { (centerOffset.x * currentScale.x) / PHYSICS_PPM, (centerOffset.y * currentScale.y) / PHYSICS_PPM };

                polyShape->SetAsBox(scaledHalfW, scaledHalfH, scaledOffset, 0.0f);
                synchronizeMassData();
            }
        }
    }

    /**
     * @brief Gets the complete collision layer bitset
     */
    uint8_t GetCollisionLayerRaw() const { return static_cast<uint8_t>(m_layer.to_ulong()); }

    /**
     * @brief Gets the complete collision mask bitset
     */
    uint8_t GetCollisionMaskRaw() const { return static_cast<uint8_t>(m_mask.to_ulong()); }

    /**
     * @brief Overwrites the entire collision layer bitset
     * @param rawLayer Byte configuration setting layer flags
     */
    void SetCollisionLayerRaw(uint8_t rawLayer) { m_layer = std::bitset<8>(rawLayer); synchronizeFilterData(); }

    /**
     * @brief Overwrites the entire collision mask bitset
     * @param rawMask Byte configuration setting mask flags
     */
    void SetCollisionMaskRaw(uint8_t rawMask) { m_mask = std::bitset<8>(rawMask); synchronizeFilterData(); }

    /**
     * @brief Assigns a single bit state flag into collision layer
     * @param index Targeted specific bit index coordinate (0 to 7)
     * @param value True turns the tracking bit active, false sets the flag clear
     */
    void SetCollisionLayer(uint8_t index, bool value) { if (index < 8) { m_layer.set(index, value); synchronizeFilterData(); } }

    /**
     * @brief Assigns a single bit state flag into collision mask
     * @param index Targeted specific bit index coordinate (0 to 7)
     * @param value True turns the tracking bit active, false sets the flag clear.
     */
    void SetCollisionMask(uint8_t index, bool value) { if (index < 8) { m_mask.set(index, value); synchronizeFilterData(); } }

    /**
     * @brief Reads a designated bit state value from the collision layer
     * @param index Targeted specific bit index channel checking (0 to 7)
     */
    bool GetCollisionLayer(uint8_t index) const { return index < 8 && m_layer.test(index); }

    /**
     * @brief Reads a designated bit state value from the collision mask
     * @param index Targeted specific bit index channel checking (0 to 7)
     */
    bool GetCollisionMask(uint8_t index) const { return index < 8 && m_mask.test(index); }

    /**
     * @brief Returns whether this is a static (immovable) body or not
     */
    bool IsStatic() const { return m_isStatic; }

    /**
     * @brief Sets the body as static (immovable) or dynamic (movable)
     * @param isStatic True makes it stationary, false enables movement
     */
    void SetStatic(bool isStatic)
    {
        m_isStatic = isStatic;
        if (body)
        {
            body->SetType(m_isStatic ? b2_staticBody : b2_dynamicBody);
            synchronizeMassData();
        }
    }

    /**
     * @brief Returns whether this body bypasses collision resolution or not
     */
    bool IsSensor() const { return m_isSensor; }

    /**
     * @brief Toggles whether the body should resolve collisions or act as a sensor
     * @param isSensor True makes it a sensor, false enables collision resolution
     */
    void SetSensor(bool isSensor)
    {
        m_isSensor = isSensor;
        if (fixture) fixture->SetSensor(m_isSensor);
    }

    /**
     * @brief Returns whether the body has locked rotation or not
     */
    bool LockRotation() const { return m_lockRotation; }

    /**
     * @brief Toggles whether this body is allowed to rotate or not
     * @param lock True locks the body rotation, false enables it
     */
    void SetLockRotation(bool lock)
    {
        m_lockRotation = lock;
        if (body) body->SetFixedRotation(m_lockRotation);
    }

    /**
     * @brief Gets the body's mass per unit volume
     */
    float GetDensity() const { return m_density; }

    /**
     * @brief Adjusts the body's density component
     * @param density Positive scalar density value
     */
    void SetDensity(float density)
    {
        m_density = std::max(0.0f, density);
        if (fixture)
        {
            fixture->SetDensity(m_density);
            synchronizeMassData();
        }
    }

    /**
     * @brief Gets the bounciness coefficient
     */
    float GetRestitution() const { return m_restitution; }

    /**
     * @brief Adjusts the body's bounciness coefficient
     * @param restitution Coefficient clamped between 0.0f and 1.0f
     */
    void SetRestitution(float restitution)
    {
        m_restitution = std::clamp(restitution, 0.0f, 1.0f);
        if (fixture) fixture->SetRestitution(m_restitution);
    }

    /**
     * @brief Gets the surface friction coefficient
     */
    float GetFriction() const { return m_friction; }

    /**
     * @brief Adjusts the surface friction coefficient
     * @param friction Coefficient clamped 0.0f and 1.0f
     */
    void SetFriction(float friction)
    {
        m_friction = std::clamp(friction, 0.0f, 1.0f);
        if (fixture)
        {
            fixture->SetFriction(m_friction);
            if (body)
            {
                for (b2ContactEdge* ce = body->GetContactList(); ce; ce = ce->next)
                {
                    ce->contact->ResetFriction();
                }
            }
        }
    }

    /** @brief Returns if body is in contact with floor */
    bool IsOnFloor()     const { return normal.y > 0.5f; }

    /** @brief Returns if body is in contact with ceiling */
    bool IsOnCeiling()   const { return normal.y < -0.5f; }

    /** @brief Returns if body is in contact with wall on the left */
    bool IsOnLeftWall()  const { return normal.x > 0.5f; }

    /** @brief Returns if body is in contact with wall on the right */
    bool IsOnRightWall() const { return normal.x < -0.5f; }

    /** @brief Returns whether this body is in a valid state or not */
    bool IsValid()       const { return body != nullptr; }

    /**
     * @brief Applies a constant linear continous force
     * @param f Force vector in pixel dimensions
     */
    void ApplyForce(const Vector2& f) const
    {
        if (body) body->ApplyForceToCenter({ f.x / PHYSICS_PPM, f.y / PHYSICS_PPM }, true);
    }

    /**
     * @brief Applies an instantaneous sudden impulse
     * @param impulse Impact vector in pixel dimensions
     */
    void ApplyImpulse(const Vector2& impulse) const
    {
        if (body) body->ApplyLinearImpulseToCenter({ impulse.x / PHYSICS_PPM, impulse.y / PHYSICS_PPM }, true);
    }

    /**
     * @brief Gets the body's current linear velocity
     * @return Linear velocity in pixels per second
     */
    Vector2 GetLinearVelocity() const
    {
        if (!body) return Vector2::ZERO;
        b2Vec2 v = body->GetLinearVelocity();
        return { v.x * PHYSICS_PPM, v.y * PHYSICS_PPM };
    }

    /**
     * @brief Sets the body's current linear velocity
     * @param v Target velocity in pixels per second
     */
    void SetLinearVelocity(const Vector2& v) const
    {
        if (body) body->SetLinearVelocity({ v.x / PHYSICS_PPM, v.y / PHYSICS_PPM });
    }

    /**
     * @brief Gets the body's current angular velocity
     * @return Angular velocity in radians per second
     */
    float GetAngularVelocity() const { return body ? body->GetAngularVelocity() : 0.0f; }

    /**
     * @brief Set's the body's current angular velocity
     * @param radPerSec Target velocity in radians per second
     */
    void SetAngularVelocity(float radPerSec) const { if (body) body->SetAngularVelocity(radPerSec); }

private:
    void synchronizeMassData()
    {
        if (body) body->ResetMassData();
    }

    void synchronizeFilterData()
    {
        if (fixture)
        {
            b2Filter filter;
            filter.categoryBits = static_cast<uint16>(m_layer.to_ulong());
            filter.maskBits = static_cast<uint16>(m_mask.to_ulong());
            filter.groupIndex = 0;
            fixture->SetFilterData(filter);
        }
    }

    Shape2D m_shape;
    union
    {
        struct { float radius; } m_circle;
        struct { float halfW; float halfH; } m_box;
    };

    float m_density;
    float m_restitution;
    float m_friction;

    bool  m_isStatic;
    bool  m_isSensor;
    bool  m_lockRotation;

    std::bitset<8> m_layer;
    std::bitset<8> m_mask;
};