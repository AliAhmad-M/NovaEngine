#pragma once

#include <Core/ECS/ECS.h>
#include <Graphics/Components/Camera2D.h>
#include <Runtime/Scene/SceneManager.h>
#include <Editor/Context.h>
#include <Editor/Scene/Scene.h>
#include <string>
#include <vector>
#include <filesystem>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <functional>
#include <chrono>
#include <imgui.h>
#include <ImGuizmo.h>

namespace Nova::Editor
{
    struct SceneTab
    {
        std::string name;
        std::string filePath;
        bool isDirty = false;
        std::unique_ptr<Scene> scene;
    };

    class ScenePanel
    {
    public:
        ScenePanel(EditorContext* ctx);
        ~ScenePanel();

        void Draw();
        void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

        void NewScene();
        void OpenSceneTab(const std::string& filePath);
        void CloseAllScenes();
        void SetActiveSceneFilePath(const std::string& filePath);
        bool BuildOnly(std::function<void(bool)> onComplete = nullptr);
        void BuildScriptsNow(std::function<void(bool)> onComplete = nullptr);
        void RequestScriptBuild();
        void PollBuild();
        std::string GetLastBuildLogLine();
        void DrawBuildOutput();
        void FocusBuildOutputOnce() { m_focusBuildOutputOnce = true; }
        bool IsBuilding() const { return m_building.load(); }
        void MarkSceneAsSaved();
        void MarkSceneAsDirty();
        void DrawGizmo();
        void DrawViewOutlines(ImVec2 imgMin, ImVec2 imgSize);
        void DrawRigidBodyDebug(ImVec2 imgMin, ImVec2 imgSize);

        unsigned int GetSceneFBOId()          const { return m_fbo; }
        unsigned int GetSceneColorTexture()   const { return m_colorTex; }
        int          GetSceneViewportWidth()  const { return m_vpWidth; }
        int          GetSceneViewportHeight() const { return m_vpHeight; }

        bool HasOpenScenes() const { return !m_sceneTabs.empty(); }

        // Returns the list of open scene file paths (for session persistence)
        std::vector<std::string> GetOpenScenePaths() const;
        int GetActiveTabIndex() const { return m_activeTab; }
        void SetActiveTab(int index);

        // Returns the editor-owned camera
        Camera2D& GetEditorCamera() { return m_editorCamera; }

    private:

        // Gizmo
        ImGuizmo::OPERATION m_gizmoOp = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE      m_gizmoMode = ImGuizmo::LOCAL;
        bool                m_useGizmo = true;

        void drawTabBar();
        void drawSceneToolBar();
        void drawGrid();
        void drawSceneViewport();
        void drawNoScenePlaceholder();
        void onTabChanged(int newIndex);

        void closeScene(int index);
        void buildAndRunGame();
        void launchBuiltGame();
        bool startBuild(bool scriptsOnly, std::function<void(bool)> onComplete);
        void appendBuildLog(const std::string& text);
        void drawBuildLog();

        void createFBO(int w, int h);
        void resizeFBO(int w, int h);
        void destroyFBO();

        // Converts a screen space ImGui delta into world space and applies it to the editor camera
        void panEditorCamera(float dx, float dy);

        EditorContext* m_ctxRef = nullptr;

        ImGuiWindowFlags      m_windowFlags = 0;
        std::vector<SceneTab> m_sceneTabs;

        int m_activeTab = -1;
        int m_lastActiveTab = -1;
        int m_tabToSelect = -1;

        unsigned int m_fbo = 0;
        unsigned int m_colorTex = 0;
        unsigned int m_depthRbo = 0;
        int          m_vpWidth = 0;
        int          m_vpHeight = 0;

        Camera2D m_editorCamera;

        bool m_showGrid = true;
        bool m_showPhysicsDebug = true;
        int  m_gridSize = 32;

        std::string         m_buildLog;
        std::mutex          m_buildLogMutex;
        std::atomic<bool>   m_building{ false };
        bool                m_buildLogScrollToBottom = false;
        bool                m_focusBuildOutputOnce = false;
        bool                m_buildResultReady = false;
        bool                m_buildResultSucceeded = false;
        std::mutex          m_buildResultMutex;
        std::thread         m_buildThread;
        std::function<void(bool)> m_buildCompletion;
        bool m_scriptBuildPending = false;
        std::chrono::steady_clock::time_point m_scriptBuildDue;
        std::vector<std::function<void(bool)>> m_scriptBuildCompletions;

        ECS* m_ecsRef;
        SceneManager* m_sceneManagerRef;
    };
}