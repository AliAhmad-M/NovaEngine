#pragma once

#include <Editor/Scene/Scene.h>
#include <Editor/Project/ProjectManager.h>

#include <string>
#include <functional>

namespace Nova::Editor
{
    class ScenePanel;

    struct EditorContext
    {
        int selectedEntity = -1;
        std::string activeScenePath;

        // Currently focused scene reference
        Scene* activeSceneRef = nullptr;

        // Function to save scene
        std::function<void()> onSaveScene;

        // Scene panel reference
        ScenePanel* scenePanelRef = nullptr;
    };
}