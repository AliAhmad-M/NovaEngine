#include <Physics/Systems/RigidBodySystem2D.h>
#include <Core/Services.h>
#include <Core/Components/Transform2D.h>
#include <Physics/Components/RigidBody2D.h>

using namespace Nova;

static inline float degToRad(float deg) { return deg * (B2_PI / 180.0f); }
static inline float radToDeg(float rad) { return rad * (180.0f / B2_PI); }

static inline b2Vec2 pointToMeters(float xPx, float yPx)
{
    return { xPx / PHYSICS_PPM, yPx / PHYSICS_PPM };
}

static b2Vec2 worldCenterToMeters(const Transform2D* t)
{
    return { t->position.x / PHYSICS_PPM, t->position.y / PHYSICS_PPM };
}

static Vector2 metersToPoint(const b2Vec2& meters)
{
    return { meters.x * PHYSICS_PPM, meters.y * PHYSICS_PPM };
}

// Shape center offset from the transform origin, in meters, in the entity's local (unrotated) space
static b2Vec2 shapeOffsetMeters(const Transform2D* t, const RigidBody2D* rb)
{
    return
    {
        (rb->centerOffset.x * t->scale.x) / PHYSICS_PPM,
        (rb->centerOffset.y * t->scale.y) / PHYSICS_PPM
    };
}

// Body origin (= shape center) in world meters for a given transform and rotation
static b2Vec2 bodyOriginFromTransform(const Transform2D* t, const RigidBody2D* rb)
{
    b2Vec2 o = shapeOffsetMeters(t, rb);
    b2Vec2 p = worldCenterToMeters(t);
    return { p.x + o.x, p.y + o.y };
}

static b2Filter makeFilter(const RigidBody2D* rb)
{
    b2Filter f = b2DefaultFilter();
    f.categoryBits = static_cast<uint64_t>(rb->GetCollisionLayerRaw());
    f.maskBits = static_cast<uint64_t>(rb->GetCollisionMaskRaw());
    return f;
}


RigidBodySystem2D::RigidBodySystem2D()
{
    m_ecs = Services::Fetch<ECS>();

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = pointToMeters(0.0f, 9.81f * PHYSICS_PPM);
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
    b2World_SetGravity(m_worldId, pointToMeters(
        meterPerSecSq.x * PHYSICS_PPM,
        meterPerSecSq.y * PHYSICS_PPM)
    );
}

void RigidBodySystem2D::OnEntityAdded(EntityID e)
{
    auto* t = m_ecs->GetComponent<Transform2D>(e);
    auto* rb = m_ecs->GetComponent<RigidBody2D>(e);

    b2Rot rot = b2MakeRot(degToRad(t->rotation));

    // Create body definition
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = rb->IsStatic() ? b2_staticBody : b2_dynamicBody;
    bodyDef.position = bodyOriginFromTransform(t, rb);
    bodyDef.rotation = rot;
    bodyDef.fixedRotation = rb->LockRotation();
    bodyDef.gravityScale = rb->ApplyGravity() ? 1.0f : 0.0f;
    bodyDef.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(e));

    // Initialize rigidbody ID
    rb->body = b2CreateBody(m_worldId, &bodyDef);

    // Setup shape properties
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = rb->GetDensity();
    shapeDef.isSensor = rb->IsSensor();
    shapeDef.filter = makeFilter(rb);
    shapeDef.enableContactEvents = !rb->IsSensor();
    shapeDef.enableSensorEvents = rb->IsSensor();

    // Setup material properties
    b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
    material.restitution = rb->GetRestitution();
    material.friction = rb->GetFriction();
    shapeDef.material = material;

    // Circle
    if (rb->GetShape() == Shape2D::CIRCLE)
    {
        float uniformScale = std::max(t->scale.x, t->scale.y);

        b2Circle circle;
        circle.center = { 0.0f, 0.0f };
        circle.radius = (rb->GetRadius() * uniformScale) / PHYSICS_PPM;

        rb->fixture = b2CreateCircleShape(rb->body, &shapeDef, &circle);
    }

    // Box
    else
    {
        float scaledHalfW = (rb->GetHalfWidth() * t->scale.x) / PHYSICS_PPM;
        float scaledHalfH = (rb->GetHalfHeight() * t->scale.y) / PHYSICS_PPM;

        b2Polygon polygon = b2MakeBox(scaledHalfW, scaledHalfH);
        rb->fixture = b2CreatePolygonShape(rb->body, &shapeDef, &polygon);
    }

    // Apply damping properties
    b2Body_SetLinearDamping(rb->body, rb->GetLinearDamping());
    b2Body_SetAngularDamping(rb->body, rb->GetAngularDamping());

    rb->lastSyncedPosition = t->position;
    rb->lastSyncedRotation = t->rotation;
}

void RigidBodySystem2D::OnEntityRemoved(EntityID e)
{
    auto* rb = m_ecs->GetComponent<RigidBody2D>(e);

    if (rb && B2_IS_NON_NULL(rb->body))
    {
        b2DestroyBody(rb->body);
        rb->body = b2_nullBodyId;
        rb->fixture = b2_nullShapeId;
    }
}

void RigidBodySystem2D::OnAllEntitiesRemoved()
{
    for (EntityID e : entities)
    {
        // Clear ID and fixture
        auto* rb = m_ecs->GetComponent<RigidBody2D>(e);
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
    worldDef.gravity = pointToMeters(g.x, g.y);
    m_worldId = b2CreateWorld(&worldDef);
}

void RigidBodySystem2D::TeleportEntity(EntityID e)
{
    auto* t = m_ecs->GetComponent<Transform2D>(e);
    auto* rb = m_ecs->GetComponent<RigidBody2D>(e);

    if (!rb || B2_IS_NULL(rb->body)) return;

    // Teleport body
    b2Rot rot = b2MakeRot(degToRad(t->rotation));
    b2Body_SetTransform(rb->body, bodyOriginFromTransform(t, rb), rot);
    b2Body_SetAwake(rb->body, true);

    rb->lastSyncedPosition = t->position;
    rb->lastSyncedRotation = t->rotation;
}

void RigidBodySystem2D::Update(float dt)
{
    if (dt <= 0.0f || B2_IS_NULL(m_worldId)) return;

    // If a transform was changed manually (scripts, editor), move the body to it.
    // lastSynced* holds exactly what this system last wrote, so any difference
    // means something else edited the transform since
    for (EntityID e : entities)
    {
        auto* t = m_ecs->GetComponent<Transform2D>(e);
        auto* rb = m_ecs->GetComponent<RigidBody2D>(e);
        if (!t || !rb || B2_IS_NULL(rb->body)) continue;

        if (t->position.x != rb->lastSyncedPosition.x ||
            t->position.y != rb->lastSyncedPosition.y ||
            t->rotation != rb->lastSyncedRotation)
        {
            TeleportEntity(e);
        }
    }

    // Enforce the Apply Gravity flag on every body each step, so no creation,
    // load or play-mode copy path can leave a body with the wrong gravity scale
    for (EntityID e : entities)
    {
        auto* rb = m_ecs->GetComponent<RigidBody2D>(e);
        if (!rb || B2_IS_NULL(rb->body)) continue;

        const float want = rb->ApplyGravity() ? 1.0f : 0.0f;
        if (b2Body_GetGravityScale(rb->body) != want)
        {
            b2Body_SetGravityScale(rb->body, want);
            if (want > 0.0f) b2Body_SetAwake(rb->body, true);
        }
    }

    b2World_Step(m_worldId, dt, 4);

    syncFromBox2D();
    processContacts();
}

void RigidBodySystem2D::syncFromBox2D()
{
    for (EntityID e : entities)
    {
        auto* t = m_ecs->GetComponent<Transform2D>(e);
        auto* rb = m_ecs->GetComponent<RigidBody2D>(e);

        if (!rb || B2_IS_NULL(rb->body)) continue;

        b2Rot  rot = b2Body_GetRotation(rb->body);
        b2Vec2 pos = b2Body_GetPosition(rb->body);   // shape center
        b2Vec2 o = shapeOffsetMeters(t, rb);

        // Transform origin = shape center minus the offset
        t->position = metersToPoint({ pos.x - o.x, pos.y - o.y });
        t->rotation = radToDeg(b2Rot_GetAngle(rot));

        rb->lastSyncedPosition = t->position;
        rb->lastSyncedRotation = t->rotation;
    }
}

void RigidBodySystem2D::processContacts()
{
    // Reset per-frame collision state
    for (EntityID e : entities)
    {
        auto* rb = m_ecs->GetComponent<RigidBody2D>(e);
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

        auto* rbA = m_ecs->GetComponent<RigidBody2D>(eA);
        auto* rbB = m_ecs->GetComponent<RigidBody2D>(eB);

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

        auto* rbA = m_ecs->GetComponent<RigidBody2D>(eA);
        auto* rbB = m_ecs->GetComponent<RigidBody2D>(eB);

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

        auto* rbSensor = m_ecs->GetComponent<RigidBody2D>(eSensor);
        auto* rbVisitor = m_ecs->GetComponent<RigidBody2D>(eVisitor);

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

        auto* rbSensor = m_ecs->GetComponent<RigidBody2D>(eSensor);
        auto* rbVisitor = m_ecs->GetComponent<RigidBody2D>(eVisitor);

        if (rbSensor) { rbSensor->isColliding = false; rbSensor->normal = Vector2::ZERO; }
        if (rbVisitor) { rbVisitor->isColliding = false; rbVisitor->normal = Vector2::ZERO; }
    }
}