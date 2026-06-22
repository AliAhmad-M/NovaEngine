#include <Editor/Panels/ScenePanel.h>
#include <Runtime/Scene/SceneManager.h>
#include <Core/ECS/ECS.h>
#include <Core/Services.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <glad/glad.h>

using namespace Nova;
using namespace Nova::Editor;

void ScenePanel::createFBO(int w, int h)
{
    m_vpWidth = w;
    m_vpHeight = h;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Color attachment
    glGenTextures(1, &m_colorTex);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

    // Depth/stencil renderbuffer
    glGenRenderbuffers(1, &m_depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ScenePanel::resizeFBO(int w, int h)
{
    destroyFBO();
    createFBO(w, h);
}

void ScenePanel::destroyFBO()
{
    if (m_depthRbo) { glDeleteRenderbuffers(1, &m_depthRbo); m_depthRbo = 0; }
    if (m_colorTex) { glDeleteTextures(1, &m_colorTex);      m_colorTex = 0; }
    if (m_fbo) { glDeleteFramebuffers(1, &m_fbo);        m_fbo = 0; }
    m_vpWidth = m_vpHeight = 0;
}


void ScenePanel::drawSceneToolBar()
{
    // Grid toggle
    ImGui::Checkbox("Grid", &m_showGrid);
    ImGui::SameLine();

    // New scene button
    if (ImGui::SmallButton("+ New Scene"))
        newScene();
}

void ScenePanel::drawTabBar()
{
    if (m_sceneTabs.empty()) return;

    ImGuiTabBarFlags tabFlags = ImGuiTabBarFlags_Reorderable |
        ImGuiTabBarFlags_AutoSelectNewTabs |
        ImGuiTabBarFlags_FittingPolicyScroll;

    if (!ImGui::BeginTabBar("##SceneTabs", tabFlags)) return;

    for (int i = 0; i < (int)m_sceneTabs.size(); ++i)
    {
        SceneTab& tab = m_sceneTabs[i];

        // Show unsaved changes indicator
        char label[256];
        std::snprintf(label, sizeof(label), "%s%s###tab%d",
            tab.isDirty ? "* " : "",
            tab.name.c_str(), i);

        ImGuiTabItemFlags itemFlags = ImGuiTabItemFlags_None;
        if (tab.isDirty)
            itemFlags |= ImGuiTabItemFlags_UnsavedDocument;

        bool open = true;
        if (ImGui::BeginTabItem(label, &open, itemFlags))
        {
            if (m_activeTab != i)
            {
                m_activeTab = i;
                onTabChanged(i);
            }

            // Sync context (unchanged)
            if (!tab.filePath.empty())
            {
                m_ctx->activeScenePath = tab.filePath;
            }
            else
            {
                std::filesystem::path dir = std::filesystem::absolute(
                    std::filesystem::current_path() / ".." / "Test" / "project" / "scenes");
                m_ctx->activeScenePath = (dir / (tab.name + ".json")).string();
            }
            m_ctx->activeScene = tab.scene.get();
            ImGui::EndTabItem();
        }

        // Tab closed via the X button
        if (!open)
        {
            // TODO: prompt save scene
            closeScene(i);
            break;
        }
    }

    ImGui::EndTabBar();
}

void ScenePanel::drawSceneViewport()
{
    ImVec2 viewSize = ImGui::GetContentRegionAvail();
    int w = (int)viewSize.x;
    int h = (int)viewSize.y;
    if (w < 1 || h < 1) return;

    // Resize FBO if the panel size changed
    if (w != m_vpWidth || h != m_vpHeight)
        resizeFBO(w, h);

    // Display the rendered scene texture.
    ImGui::Image(
        (ImTextureID)(intptr_t)m_colorTex,
        viewSize,
        ImVec2(0, 1),   // uv0: top-left of image  = bottom of texture
        ImVec2(1, 0)    // uv1: bottom-right of image = top of texture
    );

    // Pan (right/middle drag) and zoom (scroll)
    if (ImGui::IsItemActive())
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
        {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            m_cameraOffset[0] += delta.x;
            m_cameraOffset[1] += delta.y;
        }
    }
    if (ImGui::IsItemHovered())
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
        {
            m_viewportZoom *= (1.0f + wheel * 0.1f);
            m_viewportZoom = std::max(0.1f, std::min(m_viewportZoom, 10.0f));
        }
    }
}

void ScenePanel::onTabChanged(int index)
{
    if (index < 0 || index >= (int)m_sceneTabs.size()) return;

    // Deselect entity
    m_ctx->selectedEntity = -1;

    SceneTab& tab = m_sceneTabs[index];
    ECS* ecs = Services::Fetch<ECS>();
    if (!ecs) return;

    SceneManager* sceneManager = Services::Fetch<SceneManager>();

    if (!tab.filePath.empty())
    {
        // Load scene from disk
        std::vector<LoadedEntity> loaded;
        if (sceneManager->GetLoader()->Load(tab.filePath, &loaded))
        {
            tab.scene->GetEntities().clear();
            for (const auto& e : loaded)
                tab.scene->AddEntity(e.id, e.name);

            tab.isDirty = false;
        }
    }
    else
    {
        ecs->DeleteAllEntities();
        tab.scene->GetEntities().clear();
    }
}

ScenePanel::ScenePanel(Context* ctx)
    : m_ctx(ctx)
{

}

ScenePanel::~ScenePanel()
{
    destroyFBO();
}

void ScenePanel::Draw()
{
    ImGui::Begin("Scene");

    drawSceneToolBar();
    ImGui::Separator();
    drawTabBar();
    ImGui::Separator();
    drawSceneViewport();

    ImGui::End();
}

void ScenePanel::newScene()
{
    int idx = (int)m_sceneTabs.size();
    char name[64];
    std::snprintf(name, sizeof(name), "Untitled_%d", idx);

    SceneTab tab;
    tab.name = name;
    tab.filePath = "";
    tab.isDirty = true;  // Mark new scenes as dirty (unsaved)
    tab.scene = std::make_unique<Scene>(name);

    m_sceneTabs.push_back(std::move(tab));
}

void ScenePanel::closeScene(int index)
{
    if (index < 0 || index >= (int)m_sceneTabs.size()) return;
    m_sceneTabs.erase(m_sceneTabs.begin() + index);
    if (m_activeTab >= (int)m_sceneTabs.size())
        m_activeTab = (int)m_sceneTabs.size() - 1;
}

void ScenePanel::OpenSceneTab(const std::string& filePath)
{
    // If already open, just switch to it
    for (int i = 0; i < (int)m_sceneTabs.size(); i++)
    {
        if (m_sceneTabs[i].filePath == filePath)
        {
            m_lastActiveTab = -1;
            m_activeTab = i - 1;
            return;
        }
    }

    std::string name = std::filesystem::path(filePath).stem().string();

    SceneTab tab;
    tab.name = name;
    tab.filePath = filePath;
    tab.isDirty = false;
    tab.scene = std::make_unique<Scene>(name);
    tab.scene->SetFilePath(filePath);

    m_sceneTabs.push_back(std::move(tab));
}

void ScenePanel::MarkSceneAsSaved()
{
    if (m_activeTab >= 0 && m_activeTab < (int)m_sceneTabs.size())
        m_sceneTabs[m_activeTab].isDirty = false;
}

void ScenePanel::MarkSceneAsDirty()
{
    if (m_activeTab >= 0 && m_activeTab < (int)m_sceneTabs.size())
        m_sceneTabs[m_activeTab].isDirty = true;
}
