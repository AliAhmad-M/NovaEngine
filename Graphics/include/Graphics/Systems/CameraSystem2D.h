#pragma once
#include <Core/ECS/ISystem.h>
#include <Core/ECS/ECS.h>
#include <Core/EngineContext.h>

struct CameraSystem2D : public ISystem
{
	CameraSystem2D(EngineContext* ctx) : m_ctx(ctx) {}

	void Update(float dt) override;
	void OnEntityAdded(EntityID e) override {}
	void OnEntityRemoved(EntityID e) override {}
	void OnAllEntitiesRemoved() override {}

private:
	EngineContext* m_ctx;
};