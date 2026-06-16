#pragma once
#include <box2d/box2d.h>
#include <box2d/b2_world.h>
#include <box2d/b2_world_callbacks.h>
#include <box2d/b2_contact.h>
#include <vector>
#include <Core/ECS/ECS.h>
#include <Core/Components/Transform2D.h>
#include <Physics/Components/RigidBody2D.h>

// Listens for contact information
class PhysicsContactListener : public b2ContactListener
{
public:
    struct ContactEvent
    {
        b2Body* bodyA;
        b2Body* bodyB;
        float   normalX;
        float   normalY;
    };

    std::vector<ContactEvent> beginEvents;
    std::vector<ContactEvent> endEvents;

    void Clear()
    {
        beginEvents.clear();
        endEvents.clear();
    }

    void BeginContact(b2Contact* contact) override
    {
        b2WorldManifold wm;
        contact->GetWorldManifold(&wm);
        beginEvents.push_back({
            contact->GetFixtureA()->GetBody(),
            contact->GetFixtureB()->GetBody(),
            wm.normal.x,
            wm.normal.y
        });
    }

    void EndContact(b2Contact* contact) override
    {
        endEvents.push_back({
            contact->GetFixtureA()->GetBody(),
            contact->GetFixtureB()->GetBody(),
            0.0f, 0.0f
        });
    }
};

class RigidBodySystem2D : public ISystem
{
public:
   
    RigidBodySystem2D(EngineContext* ctx, Vector2 gravity = { 0.0f, 980.0f });
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

    b2World* GetWorld() const { return m_world; }

private:
    void BuildBody(EntityID e, const Transform2D* transform, RigidBody2D* rb);

    void SyncFromBox2D();
    void ProcessContacts();

    EngineContext* m_ctx;
    b2World* m_world = nullptr;
    PhysicsContactListener  m_listener;
};