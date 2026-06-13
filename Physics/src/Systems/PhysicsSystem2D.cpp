#include <Physics/Systems/PhysicsSystem2D.h>
#include <Physics/Components/Physics2D.h>
#include <Core/Components/Transform2D.h>
#include <cmath>

#define METER 32

void PhysicsSystem2D::Update(float dt)
{
    ECS* ecs = m_ctx->Get<ECS>();

    if (dt <= 0.0f) return;

    for (const EntityID e : m_entities)
    {
        auto* transform = ecs->GetComponent<Transform2D>(e);
        auto* rigidbody = ecs->GetComponent<Physics2D>(e);

        if (rigidbody->isStatic) continue;

        // Calculate semi-implicit acceleration from accumulated frames forces
        Vector2 totalAcceleration = rigidbody->acceleration + (rigidbody->force * rigidbody->GetInverseMass());

        // Integrate velocity
        rigidbody->linearVelocity += totalAcceleration * METER * dt;

        // Apply exponential linear damping
        if (rigidbody->linearDamping > 0.0f)
        {
            // v = v * e^(-damping * dt)
            rigidbody->linearVelocity = rigidbody->linearVelocity * std::exp(-rigidbody->linearDamping * dt);
        }

        // Integrate position
        transform->position += rigidbody->linearVelocity * dt;

        // Clear accumulated parameters for next frame
        rigidbody->force = Vector2::ZERO;
        rigidbody->acceleration = Vector2::ZERO;
    }
}

void PhysicsSystem2D::OnEntityAdded(EntityID e)
{
    m_entities.push_back(e);
}

void PhysicsSystem2D::OnEntityRemoved(EntityID e)
{
    auto position = std::find(m_entities.begin(), m_entities.end(), e);
    if (position != m_entities.end())
    {
        m_entities.erase(position);
    }
}

void PhysicsSystem2D::OnAllEntitiesRemoved()
{
    m_entities.clear();
}