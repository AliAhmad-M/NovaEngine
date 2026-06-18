#pragma once

#include <string>
#include <vector>
#include <memory>
#include <Editor/EditorContext.h>
#include <Editor/Scene/EditorScene.h>
#include <imgui.h>

// Represents one scene tab/entry managed by the panel.
struct SceneTab
{
    std::string name;       // Display name
    std::string filePath;   // Path to scene
    bool isDirty = false;   // Unsaved changes
    std::unique_ptr<EditorScene> scene;  // The actual scene data
};

class ScenePanel
{
public:
    ScenePanel(EditorContext* ctx);

    void Draw();
    void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

    void MarkSceneAsSaved();
    void MarkSceneAsDirty();

private:
    void drawTabBar();
    void drawSceneToolBar();
    void drawSceneViewport();

    void newScene();
    void closeScene(int index);

    EditorContext* m_ctx = nullptr;
    ImGuiWindowFlags      m_windowFlags = 0;
    std::vector<SceneTab> m_sceneTabs;
    int                   m_activeTab = -1;

    // Viewport / camera stub
    float m_viewportZoom = 1.0f;
    float m_cameraOffset[2] = { 0.0f, 0.0f };
    bool  m_showGrid = true;
    int   m_gridSize = 32;
};
