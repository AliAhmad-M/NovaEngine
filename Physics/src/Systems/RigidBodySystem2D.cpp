#include <Physics/Systems/RigidBodySystem2D.h>
#include <Core/Debug.h>
#include <Core/Services.h>
#include <Core/Components/Transform2D.h>
#include <Physics/Components/RigidBody2D.h>

#include <vector>
#include <algorithm>
#include <cstdint>
#include <cfloat>

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

static b2BodyType toBox2DType(RigidBodyType2D type)
{
    switch (type)
    {
    case RigidBodyType2D::Static:    return b2_staticBody;
    case RigidBodyType2D::Kinematic: return b2_kinematicBody;
    default:                    return b2_dynamicBody;
    }
}

static EntityID entityFromBody(b2BodyId body)
{
    return static_cast<EntityID>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(body)));
}


// Kinematic character mover
namespace
{
    constexpr int   kMaxMoverPlanes = 8;
    constexpr int   kMoverSolveIterations = 5;
    constexpr float kMoverTolerance = 0.001f; // meters

    struct MoverPlanes
    {
        b2CollisionPlane planes[kMaxMoverPlanes];
        int count = 0;
    };

    struct KinematicMoveResult
    {
        EntityID entity = 0;
        bool colliding = false;
        Vector2 normal = Vector2::ZERO;
    };

    bool collectMoverPlane(b2ShapeId shapeId, const b2PlaneResult* result, void* context)
    {
        MoverPlanes* out = static_cast<MoverPlanes*>(context);
        if (out->count >= kMaxMoverPlanes) return false;
        if (b2Shape_IsSensor(shapeId)) return true;

        b2BodyId body = b2Shape_GetBody(shapeId);

        b2CollisionPlane cp = {};
        cp.plane = result->plane;
        cp.pushLimit = (b2Body_GetType(body) == b2_dynamicBody) ? 0.0f : FLT_MAX;
        cp.push = 0.0f;
        cp.clipVelocity = true;

        out->planes[out->count++] = cp;
        return true;
    }

    b2Capsule makeMoverCapsule(const RigidBody2D* rb, const Transform2D* t, b2Vec2 center)
    {
        float radius;
        float halfSegment;

        if (rb->GetShape() == Shape2D::CIRCLE)
        {
            radius = (rb->GetRadius() * std::max(t->scale.x, t->scale.y)) / PHYSICS_PPM;
            halfSegment = 0.0f;
        }
        else
        {
            float halfW = (rb->GetHalfWidth() * t->scale.x) / PHYSICS_PPM;
            float halfH = (rb->GetHalfHeight() * t->scale.y) / PHYSICS_PPM;
            radius = std::min(halfW, halfH);
            halfSegment = halfH - radius;
        }

        b2Capsule capsule;
        capsule.center1 = { center.x, center.y - halfSegment };
        capsule.center2 = { center.x, center.y + halfSegment };
        capsule.radius = radius;
        return capsule;
    }

    void gatherMoverPlanes(b2WorldId world, const RigidBody2D* rb, const Transform2D* t,
        b2Vec2 center, b2QueryFilter filter, MoverPlanes& out)
    {
        out.count = 0;
        b2Capsule capsule = makeMoverCapsule(rb, t, center);
        b2World_CollideMover(world, &capsule, filter, collectMoverPlane, &out);
    }

    b2Vec2 moveKinematicBody(b2WorldId world, const Transform2D* t, RigidBody2D* rb,
        float dt, KinematicMoveResult& result)
    {
        b2Vec2 start = bodyOriginFromTransform(t, rb);
        b2Vec2 velocity = { rb->moveVelocity.x / PHYSICS_PPM, rb->moveVelocity.y / PHYSICS_PPM };
        b2Vec2 translation = { velocity.x * dt, velocity.y * dt };
        b2Vec2 target = b2Add(start, translation);

        // Hide body shape
        b2Filter ownFilter = b2Shape_GetFilter(rb->fixture);
        b2Filter hidden = ownFilter;
        hidden.categoryBits = 0;
        b2Shape_SetFilter(rb->fixture, hidden);

        b2QueryFilter queryFilter = b2DefaultQueryFilter();
        queryFilter.categoryBits = static_cast<uint64_t>(rb->GetCollisionLayerRaw());
        queryFilter.maskBits = static_cast<uint64_t>(rb->GetCollisionMaskRaw());

        // Sweep
        b2Capsule capsule = makeMoverCapsule(rb, t, start);
        float fraction = b2World_CastMover(world, &capsule, translation, queryFilter);

        b2Counters counters = b2World_GetCounters(world);
        b2Vec2 p = b2MulAdd(start, fraction, translation);

        // Slide
        MoverPlanes planes;
        for (int i = 0; i < kMoverSolveIterations; i++)
        {
            gatherMoverPlanes(world, rb, t, p, queryFilter, planes);
            if (planes.count == 0) break;

            b2PlaneSolverResult solved = b2SolvePlanes(b2Sub(target, p), planes.planes, planes.count);
            p = b2Add(p, solved.translation);

            if (b2LengthSquared(solved.translation) < kMoverTolerance * kMoverTolerance) break;
        }

        // Contacts
        gatherMoverPlanes(world, rb, t, p, queryFilter, planes);

        // Remove the velocity component pointing into each surface
        b2Vec2 clipped = velocity;
        for (int pass = 0; pass < 2; pass++)
        {
            for (int i = 0; i < planes.count; i++)
            {
                b2Vec2 n = planes.planes[i].plane.normal;
                float into = b2Dot(clipped, n);
                if (into < 0.0f) clipped = b2MulAdd(clipped, -into, n);
            }
        }

        rb->slideVelocity = { clipped.x * PHYSICS_PPM, clipped.y * PHYSICS_PPM };

        // Normals point away from the surface
        result.colliding = planes.count > 0;
        result.normal = Vector2::ZERO;
        for (int i = 0; i < planes.count; i++)
        {
            Vector2 n = { planes.planes[i].plane.normal.x, planes.planes[i].plane.normal.y };
            if (i == 0 || n.y < result.normal.y) result.normal = n;
        }

        b2Shape_SetFilter(rb->fixture, ownFilter);
        return p;
    }
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
    bodyDef.type = toBox2DType(rb->GetBodyType());
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
    shapeDef.enableSensorEvents = true; // lets this shape be detected by sensors

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

    if (!t || !rb || B2_IS_NULL(rb->body)) return;

    // Teleport body
    b2Rot rot = b2MakeRot(degToRad(t->rotation));
    b2Body_SetTransform(rb->body, bodyOriginFromTransform(t, rb), rot);
    b2Body_SetLinearVelocity(rb->body, { 0.0f, 0.0f });
    b2Body_SetAngularVelocity(rb->body, 0.0f);
    b2Body_SetAwake(rb->body, true);

    rb->lastSyncedPosition = t->position;
    rb->lastSyncedRotation = t->rotation;
}

void RigidBodySystem2D::Update(float dt)
{
    if (dt <= 0.0f || B2_IS_NULL(m_worldId)) return;

    // Kinematic bodies
    static std::vector<EntityID> drivenByTransform;
    drivenByTransform.clear();

    // Results of kinematic controller moves this frame
    static std::vector<KinematicMoveResult> moveResults;
    moveResults.clear();

    for (EntityID e : entities)
    {
        auto* t = m_ecs->GetComponent<Transform2D>(e);
        auto* rb = m_ecs->GetComponent<RigidBody2D>(e);
        if (!t || !rb || B2_IS_NULL(rb->body)) continue;

        const bool changed =
            t->position.x != rb->lastSyncedPosition.x ||
            t->position.y != rb->lastSyncedPosition.y ||
            t->rotation != rb->lastSyncedRotation;

        // Kinematic controller
        if (rb->moveRequested)
        {
            rb->moveRequested = false;

            if (rb->GetBodyType() == RigidBodyType2D::Kinematic)
            {
                // A manual transform edit
                if (changed) TeleportEntity(e);

                KinematicMoveResult result;
                result.entity = e;
                b2Vec2 origin = moveKinematicBody(m_worldId, t, rb, dt, result);

                // Write the new position
                b2Vec2 offset = shapeOffsetMeters(t, rb);
                t->position = metersToPoint({ origin.x - offset.x, origin.y - offset.y });
                rb->lastSyncedPosition = t->position;
                rb->lastSyncedRotation = t->rotation;

                b2Transform target;
                target.p = origin;
                target.q = b2MakeRot(degToRad(t->rotation));
                b2Body_SetTargetTransform(rb->body, target, dt);
                b2Body_SetAwake(rb->body, true);

                drivenByTransform.push_back(e);
                moveResults.push_back(result);
                continue;
            }
        }

        if (!changed) continue;

        if (rb->GetBodyType() == RigidBodyType2D::Kinematic)
        {
            // Drive the body to the new transform
            b2Transform target;
            target.p = bodyOriginFromTransform(t, rb);
            target.q = b2MakeRot(degToRad(t->rotation));
            b2Body_SetTargetTransform(rb->body, target, dt);
            b2Body_SetAwake(rb->body, true);

            rb->lastSyncedPosition = t->position;
            rb->lastSyncedRotation = t->rotation;
            drivenByTransform.push_back(e);
        }
        else
        {
            TeleportEntity(e);
        }
    }

    for (EntityID e : entities)
    {
        auto* rb = m_ecs->GetComponent<RigidBody2D>(e);
        if (!rb || B2_IS_NULL(rb->body)) continue;
        if (rb->GetBodyType() != RigidBodyType2D::Dynamic) continue;

        const float want = rb->ApplyGravity() ? 1.0f : 0.0f;
        if (b2Body_GetGravityScale(rb->body) != want)
        {
            b2Body_SetGravityScale(rb->body, want);
            if (want > 0.0f) b2Body_SetAwake(rb->body, true);
        }
    }

    static bool s_dumped = false;
    if (!s_dumped)
    {
        s_dumped = true;
        for (EntityID e : entities)
        {
            auto* rb = m_ecs->GetComponent<RigidBody2D>(e);
            if (!rb || B2_IS_NULL(rb->fixture)) continue;
            b2AABB box = b2Shape_GetAABB(rb->fixture);
        }
    }

    b2World_Step(m_worldId, dt, 4);

    // The kinematic bodies have reached their target
    for (EntityID e : drivenByTransform)
    {
        auto* rb = m_ecs->GetComponent<RigidBody2D>(e);
        if (!rb || B2_IS_NULL(rb->body)) continue;

        b2Body_SetLinearVelocity(rb->body, { 0.0f, 0.0f });
        b2Body_SetAngularVelocity(rb->body, 0.0f);
    }

    syncFromBox2D();
    processContacts();

    // Compute kinematic collision states from the move results
    for (const KinematicMoveResult& r : moveResults)
    {
        auto* rb = m_ecs->GetComponent<RigidBody2D>(r.entity);
        if (!rb || !r.colliding) continue;

        rb->isColliding = true;
        rb->normal = r.normal;
    }
}

void RigidBodySystem2D::syncFromBox2D()
{
    for (EntityID e : entities)
    {
        auto* t = m_ecs->GetComponent<Transform2D>(e);
        auto* rb = m_ecs->GetComponent<RigidBody2D>(e);

        if (!t || !rb || B2_IS_NULL(rb->body)) continue;

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
    static std::vector<b2ContactData> contactBuffer;
    static std::vector<b2ShapeId> overlapBuffer;

    // Solid contacts
    for (EntityID e : entities)
    {
        auto* rb = m_ecs->GetComponent<RigidBody2D>(e);
        if (!rb) continue;

        rb->isColliding = false;
        rb->normal = Vector2::ZERO;

        if (B2_IS_NULL(rb->body)) continue;

        int capacity = b2Body_GetContactCapacity(rb->body);
        if (capacity <= 0) continue;

        contactBuffer.resize(capacity);
        int count = b2Body_GetContactData(rb->body, contactBuffer.data(), capacity);

        bool found = false;
        for (int i = 0; i < count; i++)
        {
            const b2ContactData& c = contactBuffer[i];

            // No point of contact
            if (c.manifold.pointCount == 0) continue;

            b2BodyId bodyA = b2Shape_GetBody(c.shapeIdA);
            bool isA = B2_ID_EQUALS(bodyA, rb->body);

            // manifold.normal points from A toward B
            Vector2 n = { c.manifold.normal.x, c.manifold.normal.y };
            Vector2 mine = isA ? Vector2(-n.x, -n.y) : n;

            // Prefer floor contacts over wall contacts
            if (!found || mine.y > rb->normal.y)
                rb->normal = mine;

            found = true;
        }

        rb->isColliding = found;
    }

    // Sensors
    for (EntityID e : entities)
    {
        auto* rbSensor = m_ecs->GetComponent<RigidBody2D>(e);
        if (!rbSensor || !rbSensor->IsSensor()) continue;
        if (B2_IS_NULL(rbSensor->body) || B2_IS_NULL(rbSensor->fixture)) continue;

        int capacity = b2Shape_GetSensorCapacity(rbSensor->fixture);
        if (capacity <= 0) continue;

        overlapBuffer.resize(capacity);
        int count = b2Shape_GetSensorOverlaps(rbSensor->fixture, overlapBuffer.data(), capacity);
        if (count <= 0) continue;

        rbSensor->isColliding = true;

        for (int i = 0; i < count; i++)
        {
            EntityID eVisitor = entityFromBody(b2Shape_GetBody(overlapBuffer[i]));
            auto* rbVisitor = m_ecs->GetComponent<RigidBody2D>(eVisitor);
            if (rbVisitor) rbVisitor->isColliding = true;
        }
    }
}