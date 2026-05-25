#pragma once

#include <Core/ECS/ECS.h>
#include <Core/ECS/ISystem.h>

struct PhysicsSystem2D : public ISystem 
{
	PhysicsSystem2D(ECS* ecs) : m_ecs(ecs) {}

	void Update(float dt) override;

	void OnEntityAdded(EntityID e) override;
	void OnEntityRemoved(EntityID e) override;
	void OnAllEntitiesRemoved() override;

private:
	ECS* m_ecs = nullptr;
	std::vector<EntityID> m_entities;
};