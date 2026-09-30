#pragma once

#include <Core/ECS/ECS.h>
#include <Graphics/Components/Camera2D.h>
#include <Runtime/Scene/SceneManager.h>
#include <Editor/Context.h>
#include <Editor/Scene/Scene.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <imgui.h>

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
        ScenePanel(Context* ctx);
        ~ScenePanel();

        void Draw();
        void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

        void OpenSceneTab(const std::string& filePath);
        void MarkSceneAsSaved();
        void MarkSceneAsDirty();

        unsigned int GetSceneFBOId()          const { return m_fbo; }
        unsigned int GetSceneColorTexture()   const { return m_colorTex; }
        int          GetSceneViewportWidth()  const { return m_vpWidth; }
        int          GetSceneViewportHeight() const { return m_vpHeight; }

        unsigned int GetGameFBOId()          const { return m_gameFbo; }
        int          GetGameViewportWidth()  const { return m_gameVpWidth; }
        int          GetGameViewportHeight() const { return m_gameVpHeight; }
        bool         IsPlaying()             const { return m_playing; }

        // Returns the editor-owned camera
        Camera2D& GetEditorCamera() { return m_editorCamera; }

    private:

        void drawTabBar();
        void drawSceneToolBar();
        void drawGrid();
        void drawSceneViewport();
        void onTabChanged(int newIndex);

        void newScene();
        void closeScene(int index);
        void buildAndRunGame();
        void drawBuildLog();

        void createFBO(int w, int h);
        void resizeFBO(int w, int h);
        void destroyFBO();

        // Converts a screen-space ImGui delta into world-space and applies it to the editor camera
        void panEditorCamera(float dx, float dy);

        Context* m_ctxRef = nullptr;

        ImGuiWindowFlags      m_windowFlags = 0;
        std::vector<SceneTab> m_sceneTabs;

        int m_activeTab     = -1;
        int m_lastActiveTab = -1;

        unsigned int m_fbo = 0;
        unsigned int m_colorTex = 0;
        unsigned int m_depthRbo = 0;
        int          m_vpWidth  = 0;
        int          m_vpHeight = 0;

        Camera2D m_editorCamera;   // editor-owned, never touches ECS

        bool m_showGrid = true;
        int  m_gridSize = 32;

        std::string         m_buildLog;
        std::mutex          m_buildLogMutex;
        std::atomic<bool>   m_building{ false };
        bool                m_showBuildLog = false;
        bool                m_buildLogScrollToBottom = false;

        void startPlay();
        void stopPlay();
        void drawGameWindow();

        void createGameFBO(int w, int h);
        void resizeGameFBO(int w, int h);
        void destroyGameFBO();

        // Play mode
        bool        m_playing = false;
        bool        m_showGameWindow = false;
        std::string m_playSnapshotPath;

        unsigned int m_gameFbo = 0;
        unsigned int m_gameColorTex = 0;
        unsigned int m_gameDepthRbo = 0;
        int          m_gameVpWidth = 0;
        int          m_gameVpHeight = 0;

        ECS* m_ecsRef;
        SceneManager* m_sceneManagerRef;
    };
}