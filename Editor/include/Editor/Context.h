#pragma once
#include <Editor/Scene/Scene.h>
#include <string>
#include <functional>

namespace Nova::Editor 
{
	class ScenePanel;

	// Shared context across multiple panels
	struct Context
	{
		int selectedEntity = -1;
		std::string activeScenePath;

		// Reference to currently focused scene in editor
		Scene* activeSceneRef = nullptr;

		// Reference to scene panel for marking dirty state
		ScenePanel* scenePanelRef = nullptr;

		// Callback to trigger save from any panel
		std::function<void()> onSaveScene;
	};
}
