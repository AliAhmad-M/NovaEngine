#pragma once
#include <Core/ECS/ISystem.h>
#include <Core/ECS/ECS.h>
#include <World/Components/Tilemap2D.h>

namespace Nova 
{
	class TilemapSystem2D : public ISystem
	{
	public:
		void Update(float dt) override {}
		void OnEntityAdded(EntityID e) override;
		void OnEntityRemoved(EntityID e) override;
		void OnAllEntitiesRemoved() override {}
	};
}