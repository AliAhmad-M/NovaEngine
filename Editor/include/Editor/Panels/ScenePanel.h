#pragma once

#include <string>
#include <vector>
#include <memory>
#include <Editor/Context.h>
#include <Editor/Scene/Scene.h>
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

        void MarkSceneAsSaved();
        void MarkSceneAsDirty();

        unsigned int GetFBOId()        const { return m_fbo; }
        unsigned int GetColorTexture() const { return m_colorTex; }
        int          GetViewportWidth()  const { return m_vpWidth; }
        int          GetViewportHeight() const { return m_vpHeight; }

    private:
        void drawTabBar();
        void drawSceneToolBar();
        void drawSceneViewport();

        void newScene();
        void closeScene(int index);

        void createFBO(int w, int h);
        void resizeFBO(int w, int h);
        void destroyFBO();

        Context* m_ctx = nullptr;
        ImGuiWindowFlags     m_windowFlags = 0;
        std::vector<SceneTab> m_sceneTabs;
        int                  m_activeTab = -1;

        unsigned int m_fbo = 0;
        unsigned int m_colorTex = 0;
        unsigned int m_depthRbo = 0;
        int          m_vpWidth = 0;
        int          m_vpHeight = 0;

        float m_viewportZoom = 1.0f;
        float m_cameraOffset[2] = { 0.0f, 0.0f };
        bool  m_showGrid = true;
        int   m_gridSize = 32;
    };
}