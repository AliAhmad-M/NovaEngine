#pragma once

#include <memory>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Runtime/Engine.h>
#include <Editor/Context.h>
#include <Editor/Panels/AssetBrowserPanel.h>
#include <Editor/Panels/HierarchyPanel.h>
#include <Editor/Panels/InspectorPanel.h>
#include <Editor/Panels/ScenePanel.h>
#include <Editor/Scene/SceneSerializer.h>

namespace Nova::Editor 
{
    class EditorApp
    {
    public:
        EditorApp();
        ~EditorApp();

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

        bool m_running            = true;
        bool m_layoutInitialized  = false;
        bool m_openSceneRequested = false;
        double m_lastFrameTime    = 0.0;

        GLFWwindow* m_nativeWindow = nullptr;

        Engine m_runtime;
        Context m_ctx;
        std::unique_ptr<SceneSerializer> m_sceneSerializer;

        static constexpr float kHierarchyWidthRatio = 0.18f;
        static constexpr float kInspectorWidthRatio = 0.22f;
        static constexpr float kAssetBrowserHeightRatio = 0.28f;

        std::unique_ptr<AssetBrowserPanel> m_assetBrowserPanel;
        std::unique_ptr<HierarchyPanel>    m_hierarchyPanel;
        std::unique_ptr<InspectorPanel>    m_inspectorPanel;
        std::unique_ptr<ScenePanel>        m_scenePanel;
    };
}