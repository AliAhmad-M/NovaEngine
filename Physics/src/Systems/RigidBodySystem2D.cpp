#include <Physics/Systems/RigidBodySystem2D.h>
#include <Core/Components/Transform2D.h>
#include <Physics/Components/RigidBody2D.h>

static inline b2Vec2 PointToB2(float xPx, float yPx)
{
    return { xPx / PHYSICS_PPM, yPx / PHYSICS_PPM };
}

static b2Vec2 WorldCenterToB2(const Transform2D* t)
{
    return { t->position.x / PHYSICS_PPM, t->position.y / PHYSICS_PPM };
}

static Vector2 B2PosToEnginePos(const b2Vec2& bodyMeters)
{
    return { bodyMeters.x * PHYSICS_PPM, bodyMeters.y * PHYSICS_PPM };
}

static b2Filter MakeFilter(const RigidBody2D* rb)
{
    b2Filter f = b2DefaultFilter();
    f.categoryBits = static_cast<uint64_t>(rb->GetCollisionLayerRaw());
    f.maskBits = static_cast<uint64_t>(rb->GetCollisionMaskRaw());
    return f;
}

RigidBodySystem2D::RigidBodySystem2D(EngineContext* ctx, Vector2 gravity)
    : m_ctx(ctx)
{
    if (m_ctx == nullptr) 
    {
    
    }

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = PointToB2(gravity.x, gravity.y);
    m_worldId = b2CreateWorld(&worldDef);
}

RigidBodySystem2D::~RigidBodySystem2D()
{
    b2DestroyWorld(m_worldId);
    m_worldId = b2_nullWorldId;
}

Vector2 RigidBodySystem2D::GetGravity() const
{
    b2Vec2 g = b2World_GetGravity(m_worldId);
    return { g.x * PHYSICS_PPM, g.y * PHYSICS_PPM };
}

void RigidBodySystem2D::SetGravity(const Vector2& meterPerSecSq)
{
    b2World_SetGravity(m_worldId, PointToB2(
        meterPerSecSq.x * PHYSICS_PPM, 
        meterPerSecSq.y * PHYSICS_PPM)
    );
}

void RigidBodySystem2D::OnEntityAdded(EntityID e)
{
    ECS* ecs = m_ctx->Get<ECS>();
    auto* transform = ecs->GetComponent<Transform2D>(e);
    auto* rb = ecs->GetComponent<RigidBody2D>(e);

    // Create body definition
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = rb->IsStatic() ? b2_staticBody : b2_dynamicBody;
    bodyDef.position = WorldCenterToB2(transform);
    bodyDef.rotation = b2MakeRot(transform->rotation * (B2_PI / 180.0f));
    bodyDef.fixedRotation = rb->LockRotation();
    bodyDef.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(e));

    // Initialize rigidbody ID
    rb->body = b2CreateBody(m_worldId, &bodyDef);

    // Setup shape properties
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = rb->GetDensity();
    shapeDef.isSensor = rb->IsSensor();
    shapeDef.filter = MakeFilter(rb);
    shapeDef.enableContactEvents = !rb->IsSensor();
    shapeDef.enableSensorEvents = rb->IsSensor();

    // Setup material properties
    b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
    material.restitution = rb->GetRestitution();
    material.friction = rb->GetFriction();
    shapeDef.material = material;

    // Offset from transform in meters
    b2Vec2 localOffsetMeters =
    {
        (rb->centerOffset.x * transform->scale.x) / PHYSICS_PPM,
        (rb->centerOffset.y * transform->scale.y) / PHYSICS_PPM
    };

    // Circle
    if (rb->GetShape() == Shape2D::CIRCLE)
    {
        float uniformScale = std::max(transform->scale.x, transform->scale.y);

        b2Circle circle;
        circle.center = localOffsetMeters;
        circle.radius = (rb->GetRadius() * uniformScale) / PHYSICS_PPM;

        rb->fixture = b2CreateCircleShape(rb->body, &shapeDef, &circle);
    }

    // Box
    else
    {
        float scaledHalfW = (rb->GetHalfWidth() * transform->scale.x) / PHYSICS_PPM;
        float scaledHalfH = (rb->GetHalfHeight() * transform->scale.y) / PHYSICS_PPM;

        b2Polygon polygon = b2MakeOffsetBox(scaledHalfW, scaledHalfH, localOffsetMeters, b2Rot_identity);
        rb->fixture = b2CreatePolygonShape(rb->body, &shapeDef, &polygon);
    }

    // Apply damping properties
    b2Body_SetLinearDamping(rb->body, rb->GetLinearDamping());
    b2Body_SetAngularDamping(rb->body, rb->GetAngularDamping());
}

void RigidBodySystem2D::OnEntityRemoved(EntityID e)
{
    ECS* ecs = m_ctx->Get<ECS>();
    auto* rb = ecs->GetComponent<RigidBody2D>(e);

    if (rb && B2_IS_NON_NULL(rb->body))
    {
        b2DestroyBody(rb->body);
        rb->body = b2_nullBodyId;
        rb->fixture = b2_nullShapeId;
    }
}

void RigidBodySystem2D::OnAllEntitiesRemoved()
{
    ECS* ecs = m_ctx->Get<ECS>();

    for (EntityID e : entities)
    {
        // Clear ID and fixture
        auto* rb = ecs->GetComponent<RigidBody2D>(e);
        if (rb)
        {
            rb->body = b2_nullBodyId;
            rb->fixture = b2_nullShapeId;
        }
    }

    // Reset physics world
    Vector2 g = GetGravity();
    b2DestroyWorld(m_worldId);

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = PointToB2(g.x, g.y);
    m_worldId = b2CreateWorld(&worldDef);
}

void RigidBodySystem2D::TeleportEntity(EntityID e)
{
    ECS* ecs = m_ctx->Get<ECS>();
    auto* transform = ecs->GetComponent<Transform2D>(e);
    auto* rb = ecs->GetComponent<RigidBody2D>(e);

    if (!rb || B2_IS_NULL(rb->body)) return;

    // Teleport body to the transform position
    b2Body_SetTransform(
        rb->body,
        WorldCenterToB2(transform),
        b2MakeRot(transform->rotation * (B2_PI / 180.0f))
    );
    b2Body_SetAwake(rb->body, true);
}

void RigidBodySystem2D::Update(float dt)
{
    if (dt <= 0.0f || B2_IS_NULL(m_worldId)) return;

    b2World_Step(m_worldId, dt, 4);

    syncFromBox2D();
    processContacts();
}

void RigidBodySystem2D::syncFromBox2D()
{
    ECS* ecs = m_ctx->Get<ECS>();

    for (EntityID e : entities)
    {
        auto* rb = ecs->GetComponent<RigidBody2D>(e);
        auto* transform = ecs->GetComponent<Transform2D>(e);

        if (!rb || B2_IS_NULL(rb->body)) continue;

        // Get position and rotation from box2D
        transform->position = B2PosToEnginePos(b2Body_GetPosition(rb->body));
        transform->rotation = b2Rot_GetAngle(b2Body_GetRotation(rb->body)) * (180.0f / B2_PI);
    }
}

void RigidBodySystem2D::processContacts()
{
    ECS* ecs = m_ctx->Get<ECS>();

    // Reset per-frame collision state
    for (EntityID e : entities)
    {
        auto* rb = ecs->GetComponent<RigidBody2D>(e);
        if (!rb) continue;
        rb->isColliding = false;
        rb->normal = Vector2::ZERO;
    }

    // Solid body contacts
    b2ContactEvents contacts = b2World_GetContactEvents(m_worldId);

    for (int i = 0; i < contacts.beginCount; i++)
    {
        const b2ContactBeginTouchEvent& ev = contacts.beginEvents[i];

        b2BodyId bodyIdA = b2Shape_GetBody(ev.shapeIdA);
        b2BodyId bodyIdB = b2Shape_GetBody(ev.shapeIdB);

        EntityID eA = static_cast<EntityID>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyIdA)));
        EntityID eB = static_cast<EntityID>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyIdB)));

        // ev.manifold.normal points from A toward B
        Vector2 n = { ev.manifold.normal.x, ev.manifold.normal.y };

        auto* rbA = ecs->GetComponent<RigidBody2D>(eA);
        auto* rbB = ecs->GetComponent<RigidBody2D>(eB);

        if (rbA) { rbA->isColliding = true; rbA->normal = -n; }
        if (rbB) { rbB->isColliding = true; rbB->normal = n; }
    }

    for (int i = 0; i < contacts.endCount; i++)
    {
        const b2ContactEndTouchEvent& ev = contacts.endEvents[i];

        b2BodyId bodyIdA = b2Shape_GetBody(ev.shapeIdA);
        b2BodyId bodyIdB = b2Shape_GetBody(ev.shapeIdB);

        EntityID eA = static_cast<EntityID>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyIdA)));
        EntityID eB = static_cast<EntityID>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyIdB)));

        auto* rbA = ecs->GetComponent<RigidBody2D>(eA);
        auto* rbB = ecs->GetComponent<RigidBody2D>(eB);

        if (rbA) { rbA->isColliding = false; rbA->normal = Vector2::ZERO; }
        if (rbB) { rbB->isColliding = false; rbB->normal = Vector2::ZERO; }
    }

    // Sensor events
    b2SensorEvents sensors = b2World_GetSensorEvents(m_worldId);

    for (int i = 0; i < sensors.beginCount; ++i)
    {
        const b2SensorBeginTouchEvent& ev = sensors.beginEvents[i];

        // ev.sensorShapeId is the sensor, ev.visitorShapeId is the overlapping body
        b2BodyId sensorBody = b2Shape_GetBody(ev.sensorShapeId);
        b2BodyId visitorBody = b2Shape_GetBody(ev.visitorShapeId);

        EntityID eSensor = static_cast<EntityID>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(sensorBody)));
        EntityID eVisitor = static_cast<EntityID>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(visitorBody)));

        auto* rbSensor = ecs->GetComponent<RigidBody2D>(eSensor);
        auto* rbVisitor = ecs->GetComponent<RigidBody2D>(eVisitor);

        // Normal stays at 0 for sensors
        if (rbSensor)  rbSensor->isColliding = true;
        if (rbVisitor) rbVisitor->isColliding = true;
    }

    for (int i = 0; i < sensors.endCount; ++i)
    {
        const b2SensorEndTouchEvent& ev = sensors.endEvents[i];

        b2BodyId sensorBody = b2Shape_GetBody(ev.sensorShapeId);
        b2BodyId visitorBody = b2Shape_GetBody(ev.visitorShapeId);

        EntityID eSensor = static_cast<EntityID>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(sensorBody)));
        EntityID eVisitor = static_cast<EntityID>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(visitorBody)));

        auto* rbSensor = ecs->GetComponent<RigidBody2D>(eSensor);
        auto* rbVisitor = ecs->GetComponent<RigidBody2D>(eVisitor);

        if (rbSensor) { rbSensor->isColliding = false; rbSensor->normal = Vector2::ZERO; }
        if (rbVisitor) { rbVisitor->isColliding = false; rbVisitor->normal = Vector2::ZERO; }
    }
}