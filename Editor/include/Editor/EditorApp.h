#pragma once

#include <memory>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Runtime/Engine.h>
#include <Editor/Context.h>
#include <Editor/Project/ProjectManager.h>
#include <Editor/Project/ProjectLauncher.h>
#include <Editor/Project/ScriptHost.h>
#include <Editor/Panels/AssetBrowserPanel.h>
#include <Editor/Panels/HierarchyPanel.h>
#include <Editor/Panels/InspectorPanel.h>
#include <Editor/Panels/ScenePanel.h>
#include <Editor/Scene/SceneSerializer.h>
#include <Editor/Panels/InputMapPanel.h>

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

        void openProject(const std::string& dir);
        void closeProject();
        void buildScripts();
        void pollScriptBuild();
        void createDefaultMainScene(const ProjectInfo& project);

        bool m_running            = true;
        bool m_layoutInitialized  = false;
        bool m_openSceneRequested = false;
        bool m_showInputMap = false;
        bool m_openScenePicker = false;
        double m_lastFrameTime    = 0.0;

        GLFWwindow* m_nativeWindow = nullptr;

        Engine m_runtime;
        EditorContext m_ctx;
        ProjectManager m_projects;
        ScriptHost m_scripts;
        std::unique_ptr<ProjectLauncher> m_launcher;

        std::unique_ptr<SceneSerializer> m_sceneSerializer;

        static constexpr float kHierarchyWidthRatio = 0.18f;
        static constexpr float kInspectorWidthRatio = 0.22f;
        static constexpr float kAssetBrowserHeightRatio = 0.28f;

        std::unique_ptr<InputMapPanel> m_inputMapPanel;
        std::unique_ptr<AssetBrowserPanel> m_assetBrowserPanel;
        std::unique_ptr<HierarchyPanel>    m_hierarchyPanel;
        std::unique_ptr<InspectorPanel>    m_inspectorPanel;
        std::unique_ptr<ScenePanel>        m_scenePanel;
        std::string m_scenesPath;
    };
}