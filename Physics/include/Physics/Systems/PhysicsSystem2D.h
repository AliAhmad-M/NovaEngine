#pragma once

#include <Core/EngineContext.h>
#include <Core/ECS/ECS.h>
#include <Core/ECS/ISystem.h>

struct PhysicsSystem2D : public ISystem 
{
	PhysicsSystem2D(EngineContext* ctx) : m_ctx(ctx) {}

	void Update(float dt) override;

	void OnEntityAdded(EntityID e) override;
	void OnEntityRemoved(EntityID e) override;
	void OnAllEntitiesRemoved() override;

private:
	EngineContext* m_ctx;
	std::vector<EntityID> m_entities;
};