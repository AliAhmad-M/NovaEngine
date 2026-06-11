#pragma once

#include <memory>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Editor/EditorContext.h>
#include <Editor/Panels/AssetBrowserPanel.h>
#include <Editor/Panels/HierarchyPanel.h>
#include <Editor/Panels/InspectorPanel.h>
#include <Editor/Panels/ScenePanel.h>

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

    GLFWwindow* m_window = nullptr;
    bool m_running = true;

    EditorContext m_ctx;

    static constexpr float kHierarchyWidthRatio = 0.18f;
    static constexpr float kInspectorWidthRatio = 0.22f;
    static constexpr float kAssetBrowserHeightRatio = 0.28f;

    std::unique_ptr<AssetBrowserPanel> m_assetBrowserPanel;
    std::unique_ptr<HierarchyPanel>    m_hierarchyPanel;
    std::unique_ptr<InspectorPanel>    m_inspectorPanel;
    std::unique_ptr<ScenePanel>        m_scenePanel;
};