#pragma once

#include <Core/EngineContext.h>
#include <Core/ECS/ECS.h>
#include <Core/ECS/ISystem.h>
#include <Core/Components/Transform2D.h>
#include <Physics/Components/Collider2D.h>
#include <Physics/Components/Physics2D.h>

#include <set>

struct CollisionSystem2D : public ISystem 
{
    CollisionSystem2D(EngineContext* ctx) : m_ctx(ctx) {}

	void Update(float dt) override;

	void OnEntityAdded(EntityID e = 0)  override;
	void OnEntityRemoved(EntityID e = 0) override;
	void OnAllEntitiesRemoved() override;

private:
    EngineContext* m_ctx;

    // Represents contact between 2 colliding objects
    struct CollisionManifold
    {
        EntityID entityA;
        EntityID entityB;
        Vector2  normal;
        float    depth;
        Vector2  contactP1;
        Vector2  contactP2;
        int      contactCount;

        CollisionManifold(EntityID a, EntityID b,const Vector2& normal, float depth, 
            const Vector2& contactP1, const Vector2& contactP2, int contactCount)
            : entityA(a), entityB(b), normal(normal), depth(depth), 
            contactP1(contactP1), contactP2(contactP2), contactCount(std::clamp(contactCount, 0, 2)) {}
    };

    // Represents AABB vertical edge
    struct Edge 
	{
		EntityID entity;
		float x;
		bool isLeft;

		Edge(EntityID entity, float x, bool isLeft) 
			: entity(entity), x(x), isLeft(isLeft) {}
	};

    // X axis edges for AABBs
    std::vector<Edge> m_edges;

    // Keep track of touching edges
    std::set<EntityID> m_touching;

    void sortEdges();

    bool checkCircleCircleCollision(
        Vector2& centerA, Vector2& centerB, 
        float radiusA, float radiusB, 
        Vector2& outNormal, float& outDepth
    );
    
    bool checkBoxBoxCollision(
        Vector2& centerA, Vector2& centerB,
        std::array<Vector2, 4>& verticesA, 
        std::array<Vector2, 4>& verticesB, Vector2& outNormal, float& outDepth
    );

    bool checkCircleBoxCollision(
        Vector2& centerC, Vector2& centerP,
        float radius, 
        std::array<Vector2, 4>& vertices, Vector2& outDepth, float& outNormal
    );

    size_t findClosestVertex(
        const Vector2& center, 
        const std::array<Vector2, 4>& vertices
    );
    
    //void pointSegmentDistance(
    //    const Vector2& point,
    //    const Vector2& a, const Vector2& b,
    //    float& outDistanceSq, Vector2& outContactPoint
    //);

    void projectVertices(
        const std::array<Vector2, 4>& vertices, 
        const Vector2& axis, float* min, float* max
    );

    void projectCircle(
        const Vector2& center, float radius, 
        const Vector2& axis, float* min, float* max
    );

    void seperateBodies(
        Transform2D* transformA, Transform2D* transformB, 
        const Vector2& normal, float depth, bool isStaticA, bool isStaticB
    );


};