#pragma once

#include <Core/Utilities/Vector2.h>
#include <Core/Utilities/Shape2D.h>
#include <box2d/box2d.h>
#include <bitset>
#include <algorithm>

static constexpr float PHYSICS_PPM = 64.0f;

struct RigidBody2D
{
public:
    b2BodyId  body = b2_nullBodyId;
    b2ShapeId fixture = b2_nullShapeId;

    Vector2 centerOffset;
    Vector2 normal = Vector2::ZERO;
    bool isColliding = false;

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
     * @param linearDamping Damping applied to linear velocity, reduces speed over time
     * @param angularDamping Damping applied to angular velocity, reduces rotation over time
     */
    RigidBody2D(
        const Vector2& centerOffset,
        float r,
        bool  isStatic = false,
        bool  isSensor = false,
        float density = 1.0f,
        float restitution = 0.2f,
        float friction = 0.5f,
        bool  lockRotation = false,
        float linearDamping = 0.0f,
        float angularDamping = 0.0f
    )
        : centerOffset(centerOffset)
        , m_shape(Shape2D::CIRCLE)
        , m_isStatic(isStatic)
        , m_isSensor(isSensor)
        , m_density(std::max(0.0f, density))
        , m_restitution(std::clamp(restitution, 0.0f, 1.0f))
        , m_friction(std::clamp(friction, 0.0f, 1.0f))
        , m_lockRotation(lockRotation)
        , m_linearDamping(std::max(0.0f, linearDamping))
        , m_angularDamping(std::max(0.0f, angularDamping))
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
     * @param linearDamping Damping applied to linear velocity, reduces speed over time
     * @param angularDamping Damping applied to angular velocity, reduces rotation over time
     */
    RigidBody2D(
        const Vector2& centerOffset,
        float w,
        float h,
        bool  isStatic = false,
        bool  isSensor = false,
        float density = 1.0f,
        float restitution = 0.2f,
        float friction = 0.5f,
        bool  lockRotation = false,
        float linearDamping = 0.0f,
        float angularDamping = 0.0f
    )
        : centerOffset(centerOffset)
        , m_shape(Shape2D::BOX)
        , m_isStatic(isStatic)
        , m_isSensor(isSensor)
        , m_density(std::max(0.0f, density))
        , m_restitution(std::clamp(restitution, 0.0f, 1.0f))
        , m_friction(std::clamp(friction, 0.0f, 1.0f))
        , m_lockRotation(lockRotation)
        , m_linearDamping(std::max(0.0f, linearDamping))
        , m_angularDamping(std::max(0.0f, angularDamping))
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
     * @param currentScale Optional uniform scale multiplier
     */
    void SetRadius(float radius, float currentScale = 1.0f)
    {
        if (m_shape != Shape2D::CIRCLE) return;
        m_circle.radius = std::max(0.001f, radius);

        if (B2_IS_NON_NULL(fixture))
        {
            b2Circle circle;
            circle.center = {
                (centerOffset.x * currentScale) / PHYSICS_PPM,
                (centerOffset.y * currentScale) / PHYSICS_PPM
            };
            circle.radius = (m_circle.radius * currentScale) / PHYSICS_PPM;
            b2Shape_SetCircle(fixture, &circle);
            synchronizeMassData();
        }
    }

    /**
     * @brief Gets half width span
     * @return Stored half width in pixels
     */
    float GetHalfWidth()  const { return m_shape == Shape2D::BOX ? m_box.halfW : 0.0f; }

    /**
     * @brief Gets full width span
     * @return Width of the body in pixels
     */
    float GetFullWidth()  const { return m_shape == Shape2D::BOX ? m_box.halfW * 2.0f : 0.0f; }

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
     * @param currentScale Optional per-axis scale multiplier
     */
    void SetDimensions(float w, float h, const Vector2& currentScale = Vector2::ONE)
    {
        if (m_shape != Shape2D::BOX) return;
        m_box.halfW = std::max(0.001f, w * 0.5f);
        m_box.halfH = std::max(0.001f, h * 0.5f);

        if (B2_IS_NON_NULL(fixture))
        {
            float   scaledHalfW = (m_box.halfW * currentScale.x) / PHYSICS_PPM;
            float   scaledHalfH = (m_box.halfH * currentScale.y) / PHYSICS_PPM;
            b2Vec2  offset = {
                (centerOffset.x * currentScale.x) / PHYSICS_PPM,
                (centerOffset.y * currentScale.y) / PHYSICS_PPM
            };
            b2Polygon polygon = b2MakeOffsetBox(scaledHalfW, scaledHalfH, offset, b2Rot_identity);
            b2Shape_SetPolygon(fixture, &polygon);
            synchronizeMassData();
        }
    }

    /**
     * @brief Gets the complete collision layer bitset
     */
    uint8_t GetCollisionLayerRaw() const { return static_cast<uint8_t>(m_layer.to_ulong()); }

    /**
     * @brief Gets the complete collision mask bitset
     */
    uint8_t GetCollisionMaskRaw()  const { return static_cast<uint8_t>(m_mask.to_ulong()); }

    /**
     * @brief Overwrites the entire collision layer bitset
     * @param rawLayer Byte configuration setting layer flags
     */
    void SetCollisionLayerRaw(uint8_t rawLayer) { m_layer = std::bitset<8>(rawLayer); synchronizeFilterData(); }

    /**
     * @brief Overwrites the entire collision mask bitset
     * @param rawMask Byte configuration setting mask flags
     */
    void SetCollisionMaskRaw(uint8_t rawMask) { m_mask = std::bitset<8>(rawMask);  synchronizeFilterData(); }

    /**
     * @brief Assigns a single bit state flag into collision layer
     * @param index Targeted specific bit index coordinate (0 to 7)
     * @param value True turns the tracking bit active, false sets the flag clear
     */
    void SetCollisionLayer(uint8_t index, bool value) { if (index < 8) { m_layer.set(index, value); synchronizeFilterData(); } }

    /**
     * @brief Assigns a single bit state flag into collision mask
     * @param index Targeted specific bit index coordinate (0 to 7)
     * @param value True turns the tracking bit active, false sets the flag clear
     */
    void SetCollisionMask(uint8_t index, bool value) { if (index < 8) { m_mask.set(index, value);  synchronizeFilterData(); } }

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
        if (B2_IS_NON_NULL(body))
        {
            b2Body_SetType(body, m_isStatic ? b2_staticBody : b2_dynamicBody);
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
     * @note The change takes effect when the body is recreated (typically on next system update)
     */
    void SetSensor(bool isSensor)
    {
        m_isSensor = isSensor;
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
        if (B2_IS_NON_NULL(body)) b2Body_SetFixedRotation(body, m_lockRotation);
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
        if (B2_IS_NON_NULL(fixture))
            b2Shape_SetDensity(fixture, m_density, true);
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
        if (B2_IS_NON_NULL(fixture)) b2Shape_SetRestitution(fixture, m_restitution);
    }

    /**
     * @brief Gets the surface friction coefficient
     */
    float GetFriction() const { return m_friction; }

    /**
     * @brief Adjusts the surface friction coefficient
     * @param friction Coefficient clamped between 0.0f and 1.0f
     */
    void SetFriction(float friction)
    {
        m_friction = std::clamp(friction, 0.0f, 1.0f);
        if (B2_IS_NON_NULL(fixture)) b2Shape_SetFriction(fixture, m_friction);
    }

    /**
     * @brief Gets the linear damping coefficient
     * @return Damping value that reduces linear velocity over time
     */
    float GetLinearDamping() const { return m_linearDamping; }

    /**
     * @brief Adjusts the linear damping coefficient
     * @param damping Non-negative damping value, reduces linear velocity over time
     */
    void SetLinearDamping(float damping)
    {
        m_linearDamping = std::max(0.0f, damping);
        if (B2_IS_NON_NULL(body)) b2Body_SetLinearDamping(body, m_linearDamping);
    }

    /**
     * @brief Gets the angular damping coefficient
     * @return Damping value that reduces angular velocity over time
     */
    float GetAngularDamping() const { return m_angularDamping; }

    /**
     * @brief Adjusts the angular damping coefficient
     * @param damping Non-negative damping value, reduces angular velocity over time
     */
    void SetAngularDamping(float damping)
    {
        m_angularDamping = std::max(0.0f, damping);
        if (B2_IS_NON_NULL(body)) b2Body_SetAngularDamping(body, m_angularDamping);
    }

    /** @brief Returns if body is in contact with floor */
    bool IsOnFloor()     const { return normal.y > 0.5f; }

    /** @brief Returns if body is in contact with ceiling */
    bool IsOnCeiling()   const { return normal.y < -0.5f; }

    /** @brief Returns if body is in contact with wall on the left */
    bool IsOnLeftWall()  const { return normal.x > 0.5f; }

    /** @brief Returns if body is in contact with wall on the right */
    bool IsOnRightWall() const { return normal.x < -0.5f; }

    /**
     * @brief Returns whether this body is in a valid state or not
     */
    bool IsValid() const { return B2_IS_NON_NULL(body); }

    /**
     * @brief Applies a constant linear continuous force
     * @param f Force vector in pixel dimensions
     */
    void ApplyForce(const Vector2& f) const
    {
        if (B2_IS_NON_NULL(body))
            b2Body_ApplyForceToCenter(body, { f.x / PHYSICS_PPM, f.y / PHYSICS_PPM }, true);
    }

	/**
	 * @brief Applies a constant angular continuous torque
	 * @param torque Torque value in pixel dimensions
	 */
	void ApplyTorque(float torque) const
	{
		if (B2_IS_NON_NULL(body))
			b2Body_ApplyTorque(body, torque, true);
	}

    /**
     * @brief Applies an instantaneous sudden impulse
     * @param impulse Impact vector in pixel dimensions
     */
    void ApplyImpulse(const Vector2& impulse) const
    {
        if (B2_IS_NON_NULL(body))
            b2Body_ApplyLinearImpulseToCenter(body, { impulse.x / PHYSICS_PPM, impulse.y / PHYSICS_PPM }, true);
    }

    /**
     * @brief Gets the body's current linear velocity
     * @return Linear velocity in pixels per second
     */
    Vector2 GetLinearVelocity() const
    {
        if (B2_IS_NULL(body)) return Vector2::ZERO;
        b2Vec2 v = b2Body_GetLinearVelocity(body);
        return { v.x * PHYSICS_PPM, v.y * PHYSICS_PPM };
    }

    /**
     * @brief Sets the body's current linear velocity
     * @param v Target velocity in pixels per second
     */
    void SetLinearVelocity(const Vector2& v) const
    {
        if (B2_IS_NON_NULL(body))
            b2Body_SetLinearVelocity(body, { v.x / PHYSICS_PPM, v.y / PHYSICS_PPM });
    }

    /**
     * @brief Gets the body's current angular velocity
     * @return Angular velocity in radians per second
     */
    float GetAngularVelocity() const
    {
        return B2_IS_NON_NULL(body) ? b2Body_GetAngularVelocity(body) : 0.0f;
    }

    /**
     * @brief Sets the body's current angular velocity
     * @param radPerSec Target velocity in radians per second
     */
    void SetAngularVelocity(float radPerSec) const
    {
        if (B2_IS_NON_NULL(body)) b2Body_SetAngularVelocity(body, radPerSec);
    }

private:
    void synchronizeMassData()
    {
        if (B2_IS_NON_NULL(body)) b2Body_ApplyMassFromShapes(body);
    }

    void synchronizeFilterData()
    {
        if (B2_IS_NON_NULL(fixture))
        {
            b2Filter filter = b2DefaultFilter();
            filter.categoryBits = static_cast<uint64_t>(m_layer.to_ulong());
            filter.maskBits = static_cast<uint64_t>(m_mask.to_ulong());
            b2Shape_SetFilter(fixture, filter);
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
    float m_linearDamping;
    float m_angularDamping;

    bool m_isStatic;
    bool m_isSensor;
    bool m_lockRotation;

    std::bitset<8> m_layer;
    std::bitset<8> m_mask;
};