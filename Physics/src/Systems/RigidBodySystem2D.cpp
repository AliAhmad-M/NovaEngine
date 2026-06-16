#include <Physics/Systems/RigidBodySystem2D.h>
#include <Core/Components/Transform2D.h>
#include <Physics/Components/RigidBody2D.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>

static inline b2Vec2 ToB2(float xPx, float yPx)
{
    return { xPx / PHYSICS_PPM, yPx / PHYSICS_PPM };
}

static b2Vec2 WorldCenterToB2(const Transform2D* t)
{
    // Convert to B2 meters
    return { t->position.x / PHYSICS_PPM, t->position.y / PHYSICS_PPM };
}

static Vector2 B2PosToEnginePos(const b2Vec2& bodyMeters)
{
    // Convert back to pixels
    return { bodyMeters.x * PHYSICS_PPM, bodyMeters.y * PHYSICS_PPM };
}

static b2Filter MakeFilter(const RigidBody2D* rb)
{
    b2Filter f;
    f.categoryBits = static_cast<uint16>(rb->GetCollisionLayerRaw());
    f.maskBits = static_cast<uint16>(rb->GetCollisionMaskRaw());
    f.groupIndex = 0;
    return f;
}

RigidBodySystem2D::RigidBodySystem2D(EngineContext* ctx, Vector2 gravity)
    : m_ctx(ctx)
{
    b2Vec2 g = ToB2(gravity.x, gravity.y);
    m_world = new b2World(g);
    m_world->SetContactListener(&m_listener);
}

RigidBodySystem2D::~RigidBodySystem2D()
{
    delete m_world;
    m_world = nullptr;
}

Vector2 RigidBodySystem2D::GetGravity() const
{
    b2Vec2 g = m_world->GetGravity();
    return { g.x * PHYSICS_PPM, g.y * PHYSICS_PPM };
}

void RigidBodySystem2D::SetGravity(const Vector2& pixelsPerSecSq)
{
    m_world->SetGravity(ToB2(pixelsPerSecSq.x, pixelsPerSecSq.y));
}

void RigidBodySystem2D::BuildBody(EntityID e, const Transform2D* transform, RigidBody2D* rb)
{
    b2BodyDef bodyDef;
    bodyDef.type = rb->IsStatic() ? b2_staticBody : b2_dynamicBody;
    bodyDef.position = WorldCenterToB2(transform);
    bodyDef.angle = transform->rotation * (b2_pi / 180.0f);
    bodyDef.fixedRotation = rb->LockRotation();
    bodyDef.userData.pointer = static_cast<uintptr_t>(e);

    rb->body = m_world->CreateBody(&bodyDef);

    b2FixtureDef fixDef;
    fixDef.density = rb->GetDensity();
    fixDef.restitution = rb->GetRestitution();
    fixDef.friction = rb->GetFriction();
    fixDef.isSensor = rb->IsSensor();
    fixDef.filter = MakeFilter(rb);

    b2CircleShape circle;
    b2PolygonShape box;

    // Calculate your scaled local offset in meters
    b2Vec2 localOffsetMeters = {
        (rb->centerOffset.x * transform->scale.x) / PHYSICS_PPM,
        (rb->centerOffset.y * transform->scale.y) / PHYSICS_PPM
    };

    if (rb->GetShape() == Shape2D::CIRCLE)
    {
        float uniformScale = std::max(transform->scale.x, transform->scale.y);

        // Position the circle relative to the body origin using the offset
        circle.m_p = localOffsetMeters;
        circle.m_radius = (rb->GetRadius() * uniformScale) / PHYSICS_PPM;
        fixDef.shape = &circle;
    }
    else
    {
        float scaledHalfW = (rb->GetHalfWidth() * transform->scale.x) / PHYSICS_PPM;
        float scaledHalfH = (rb->GetHalfHeight() * transform->scale.y) / PHYSICS_PPM;

        // Pass the local center offset directly into SetAsBox!
        box.SetAsBox(scaledHalfW, scaledHalfH, localOffsetMeters, 0.0f);
        fixDef.shape = &box;
    }

    rb->fixture = rb->body->CreateFixture(&fixDef);
}

void RigidBodySystem2D::OnEntityAdded(EntityID e)
{
    ECS* ecs = m_ctx->Get<ECS>();
    auto* transform = ecs->GetComponent<Transform2D>(e);
    auto* rb = ecs->GetComponent<RigidBody2D>(e);

    BuildBody(e, transform, rb);

    entities.insert(e);
}

void RigidBodySystem2D::OnEntityRemoved(EntityID e)
{
    ECS* ecs = m_ctx->Get<ECS>();
    auto* rb = ecs->GetComponent<RigidBody2D>(e);

    if (rb && rb->body)
    {
        m_world->DestroyBody(rb->body);
        rb->body = nullptr;
        rb->fixture = nullptr;
    }

    entities.erase(e);
}

void RigidBodySystem2D::OnAllEntitiesRemoved()
{
    Vector2 g = GetGravity();

    delete m_world;
    entities.clear();

    m_world = new b2World(ToB2(g.x, g.y));
    m_world->SetContactListener(&m_listener);
}

void RigidBodySystem2D::TeleportEntity(EntityID e)
{
    ECS* ecs = m_ctx->Get<ECS>();
    auto* transform = ecs->GetComponent<Transform2D>(e);
    auto* rb = ecs->GetComponent<RigidBody2D>(e);

    if (!rb || !rb->body) return;

    rb->body->SetTransform(WorldCenterToB2(transform), transform->rotation * (b2_pi / 180.0f));
    rb->body->SetAwake(true);
}

void RigidBodySystem2D::Update(float dt)
{
    if (dt <= 0.0f || !m_world) return;

    m_listener.Clear();
    m_world->Step(dt, 8, 3);

    SyncFromBox2D();
    ProcessContacts();
}

void RigidBodySystem2D::SyncFromBox2D()
{
    ECS* ecs = m_ctx->Get<ECS>();

    for (EntityID e : entities)
    {
        auto* rb = ecs->GetComponent<RigidBody2D>(e);
        auto* transform = ecs->GetComponent<Transform2D>(e);

        if (!rb || !rb->body) continue;
        transform->position = B2PosToEnginePos(rb->body->GetPosition());
        transform->rotation = rb->body->GetAngle() * (180.0f / b2_pi);
    }
}

void RigidBodySystem2D::ProcessContacts()
{
    ECS* ecs = m_ctx->Get<ECS>();

    // Reset per-frame state
    for (EntityID e : entities)
    {
        auto* rb = ecs->GetComponent<RigidBody2D>(e);
        if (!rb) continue;
        rb->isColliding = false;
        rb->normal = Vector2::ZERO;
    }

    // Begin contacts
    for (const auto& ev : m_listener.beginEvents)
    {
        EntityID eA = (EntityID)ev.bodyA->GetUserData().pointer;
        EntityID eB = (EntityID)ev.bodyB->GetUserData().pointer;

        Vector2 n = { ev.normalX, ev.normalY };

        auto* rbA = ecs->GetComponent<RigidBody2D>(eA);
        auto* rbB = ecs->GetComponent<RigidBody2D>(eB);

        if (rbA) { rbA->isColliding = true; rbA->normal = -n; }
        if (rbB) { rbB->isColliding = true; rbB->normal = n; }
    }

    // End contacts
    for (const auto& ev : m_listener.endEvents)
    {
        EntityID eA = (EntityID)ev.bodyA->GetUserData().pointer;
        EntityID eB = (EntityID)ev.bodyB->GetUserData().pointer;

        auto* rbA = ecs->GetComponent<RigidBody2D>(eA);
        auto* rbB = ecs->GetComponent<RigidBody2D>(eB);

        if (rbA) { rbA->isColliding = false; rbA->normal = Vector2::ZERO; }
        if (rbB) { rbB->isColliding = false; rbB->normal = Vector2::ZERO; }
    }
}