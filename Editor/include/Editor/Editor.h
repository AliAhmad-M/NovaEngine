#pragma once

#include <memory>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Runtime/EngineRuntime.h>
#include <Editor/EditorContext.h>
#include <Editor/Panels/AssetBrowserPanel.h>
#include <Editor/Panels/HierarchyPanel.h>
#include <Editor/Panels/InspectorPanel.h>
#include <Editor/Panels/ScenePanel.h>
#include <Editor/Scene/SceneSerializer.h>

class Editor
{
public:
    Editor();
    ~Editor();

    bool Init();
    void Run();
    void Shutdown();

private:
    void beginFrame();
    void endFrame();
    void buildLayout();
    void drawMenuBar();
    void saveActiveScene();  // Helper to save the current scene
    void markSceneDirty();   // Helper to mark active scene as dirty/modified

    bool m_layoutInitialized = false;

    GLFWwindow* m_window = nullptr;
    bool m_running = true;

    EngineRuntime m_runtime;
    EditorContext m_ctx;
    std::unique_ptr<SceneSerializer> m_sceneSerializer;

    static constexpr float kHierarchyWidthRatio = 0.18f;
    static constexpr float kInspectorWidthRatio = 0.22f;
    static constexpr float kAssetBrowserHeightRatio = 0.28f;

    std::unique_ptr<AssetBrowserPanel> m_assetBrowserPanel;
    std::unique_ptr<HierarchyPanel>    m_hierarchyPanel;
    std::unique_ptr<InspectorPanel>    m_inspectorPanel;
    std::unique_ptr<ScenePanel>        m_scenePanel;
};