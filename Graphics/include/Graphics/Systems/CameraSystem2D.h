#pragma once
#include <Core/ECS/ISystem.h>
#include <Core/ECS/ECS.h>
#include <Platform/Window.h>

namespace Nova 
{
	struct CameraSystem2D : public ISystem
	{
		CameraSystem2D();
		void Update(float dt = 0.0f) override;
		void OnEntityAdded(EntityID e) override {}
		void OnEntityRemoved(EntityID e) override {}
		void OnAllEntitiesRemoved() override {}

	private:
		Window* m_windowRef;
		ECS* m_ecsRef;
	};
}
