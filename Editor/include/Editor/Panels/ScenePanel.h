#pragma once

#include <Editor/Context.h>
#include <Editor/Scene/Scene.h>
#include <Graphics/Components/Camera2D.h>
#include <string>
#include <vector>
#include <memory>
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

        unsigned int GetFBOId()          const { return m_fbo; }
        unsigned int GetColorTexture()   const { return m_colorTex; }
        int          GetViewportWidth()  const { return m_vpWidth; }
        int          GetViewportHeight() const { return m_vpHeight; }

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

        void createFBO(int w, int h);
        void resizeFBO(int w, int h);
        void destroyFBO();


        // Converts a screen-space ImGui delta into world-space and applies it to the editor camera
        void panEditorCamera(float dx, float dy);

        Context* m_ctx = nullptr;

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
    };
}