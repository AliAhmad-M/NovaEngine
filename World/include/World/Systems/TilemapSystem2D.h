#pragma once
#include <Core/ECS/ISystem.h>
#include <Core/ECS/ECS.h>
#include <Core/Components/Transform2D.h>
#include <unordered_map>

namespace Nova 
{
	struct TilemapSystem2D : public ISystem
	{
		TilemapSystem2D();
		void Update(float dt = 0.0f) override;
		void OnEntityAdded(EntityID e) override;
		void OnEntityRemoved(EntityID e) override;
		void OnAllEntitiesRemoved() override;

	private:
		ECS* m_ecs;
		std::unordered_map<EntityID, Transform2D> m_cachedTransforms;

	};
}