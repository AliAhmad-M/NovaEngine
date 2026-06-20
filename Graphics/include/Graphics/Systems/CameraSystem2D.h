#pragma once
#include <Core/ECS/ISystem.h>
#include <Core/ECS/ECS.h>

namespace Nova 
{
	struct CameraSystem2D : public ISystem
	{
		void Update(float dt) override;
		void OnEntityAdded(EntityID e) override {}
		void OnEntityRemoved(EntityID e) override {}
		void OnAllEntitiesRemoved() override {}
	};
}
