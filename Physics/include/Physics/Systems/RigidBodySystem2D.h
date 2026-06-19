#pragma once
#include <box2d/box2d.h>
#include <vector>
#include <Core/ECS/ECS.h>
#include <Core/Components/Transform2D.h>
#include <Physics/Components/RigidBody2D.h>

class RigidBodySystem2D : public ISystem
{
public:
   
    RigidBodySystem2D(EngineContext* ctx, Vector2 gravity = { 0.0f, 9.81f * PHYSICS_PPM });
    ~RigidBodySystem2D();

    RigidBodySystem2D(const RigidBodySystem2D&) = delete;
    RigidBodySystem2D& operator=(const RigidBodySystem2D&) = delete;

    void Update(float dt);

    void OnEntityAdded(EntityID e);
    void OnEntityRemoved(EntityID e);
    void OnAllEntitiesRemoved();

    Vector2 GetGravity() const;
    void SetGravity(const Vector2& pixelsPerSecSq);

    void TeleportEntity(EntityID e);

    b2WorldId GetWorldId() const { return m_worldId; }

private:
    void buildBody(EntityID e, const Transform2D* transform, RigidBody2D* rb);

    void syncFromBox2D();
    void processContacts();

    EngineContext* m_ctx;
    b2WorldId m_worldId = b2_nullWorldId;
};