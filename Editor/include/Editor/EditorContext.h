#pragma once
#include <Runtime/ContentLoader.h>
#include <Editor/Scene/EditorScene.h>
#include <string>

class ScenePanel;

// Shared context across multiple panels
struct EditorContext
{
	int selectedEntity = -1;
	std::string activeScenePath;

	// The scene currently focused in the editor
	EditorScene* activeScene = nullptr;

	// Reference to scene panel for marking dirty state
	ScenePanel* scenePanel = nullptr;
};