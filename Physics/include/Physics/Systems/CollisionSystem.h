#pragma once

#include <set>

#include <Core/ECS/ECS.h>
#include <Core/ECS/ISystem.h>
#include <Core/Components/Transform2D.h>
#include <Physics/Components/Collider2D.h>

struct CollisionSystem : public ISystem 
{
    CollisionSystem(ECS* ecs) : m_ecs(ecs) {}

	void Update(float dt) override;

	void OnEntityAdded(EntityID e = 0) override;
	void OnEntityRemoved(EntityID e = 0) override;
	void OnAllEntitiesRemoved() override;

private:
	struct Edge 
	{
		EntityID entity;
		float x;
		bool isLeft;

		Edge(EntityID entity, float x, bool isLeft) 
			: entity(entity), x(x), isLeft(isLeft) {}
	};

    // Reference to the ECS
    ECS* m_ecs;

    // X axis edges for AABBs
    std::vector<Edge> m_edges;

    // Keep track of touching edges
    std::set<EntityID> m_touching;

    void sortEdges();

    bool checkCircleCircleCollision(
        Vector2& centerA, Vector2& centerB, float radiusA, float radiusB, Vector2* normal, float* depth
    );

    bool checkBoxBoxCollision(
        Vector2& centerA, Vector2& centerB,
        std::array<Vector2, 4>& verticesA, std::array<Vector2, 4>& verticesB, Vector2* normal, float* depth
    );

    bool checkCircleBoxCollision(
        Vector2& centerC, Vector2& centerP,
        float radius, std::array<Vector2, 4>& vertices, Vector2* normal, float* depth
    );

    size_t findClosestVertex(const Vector2& center, const std::array<Vector2, 4>& vertices);
    void projectVertices(const std::array<Vector2, 4>& vertices, const Vector2& axis, float* min, float* max);
    void projectCircle(const Vector2& center, float radius, const Vector2& axis, float* min, float* max);

    void seperateBodies(Transform2D* transformA, Transform2D* transformB, const Vector2& normal, float depth, bool isStaticA, bool isStaticB);
};