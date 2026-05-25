#include <Physics/Systems/PhysicsSystem2D.h>
#include <Physics/Components/Physics2D.h>
#include <Core/Components/Transform2D.h>

#define METER 32

void PhysicsSystem2D::Update(float dt)
{
    for (const EntityID e : m_entities)
    {
        auto* transform = m_ecs->GetComponent<Transform2D>(e);
        auto* rigidbody = m_ecs->GetComponent<Physics2D>(e);

        if (rigidbody->isStationary) continue;

        rigidbody->velocity += rigidbody->acceleration * METER * dt;
        transform->position += rigidbody->velocity * dt;
        rigidbody->acceleration = Vector2::ZERO;
    }
}

void PhysicsSystem2D::OnEntityAdded(EntityID e)
{
	// Add the entity
	m_entities.push_back(e);
}

void PhysicsSystem2D::OnEntityRemoved(EntityID e)
{
	// Find and delete entity
	auto position = std::find(
		m_entities.begin(),
		m_entities.end(), e
	);

	m_entities.erase(position);
}

void PhysicsSystem2D::OnAllEntitiesRemoved()
{
	m_entities.clear();
}