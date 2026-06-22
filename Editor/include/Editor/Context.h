#pragma once
#include <Runtime/ContentLoader.h>
#include <Editor/Scene/Scene.h>
#include <string>

namespace Nova::Editor 
{
	class ScenePanel;

	// Shared context across multiple panels
	struct Context
	{
		int selectedEntity = -1;
		std::string activeScenePath;

		// The scene currently focused in the editor
		Scene* activeScene = nullptr;

		// Reference to scene panel for marking dirty state
		ScenePanel* scenePanel = nullptr;
	};
}
