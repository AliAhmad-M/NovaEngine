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
    if (m_depthRbo) 
    { 
        glDeleteRenderbuffers(1, &m_depthRbo); 
        m_depthRbo = 0; 
    }
    if (m_colorTex) 
    { 
        glDeleteTextures(1, &m_colorTex);      
        m_colorTex = 0; 
    }
    if (m_fbo) 
    { 
        glDeleteFramebuffers(1, &m_fbo);            
        m_fbo = 0; 
    }
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
                m_ctx->activeScenePath = (dir / (tab.name + ".nova")).string();
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

void ScenePanel::drawGrid()
{
    ImDrawList* dl      = ImGui::GetWindowDrawList();
    ImVec2      origin  = ImGui::GetItemRectMin(); 
    ImVec2      size    = ImGui::GetItemRectSize();

    float zoom  = m_editorCamera.GetZoom();
    Vector2 off = m_editorCamera.GetOffset();

    // World-space grid step in pixels, scaled by zoom
    float step = (float)m_gridSize * zoom;
    if (step < 4.0f) return;

    // How far into the first cell the camera offset is
    float phaseX = std::fmod(off.x * zoom, step);
    float phaseY = std::fmod(off.y * zoom, step);
    if (phaseX < 0.0f) phaseX += step;
    if (phaseY < 0.0f) phaseY += step;

    ImU32 lineColor = IM_COL32(255, 255, 255, 30);
    ImU32 axisColor = IM_COL32(255, 255, 255, 80);

    // Vertical lines
    for (float x = origin.x - phaseX; x <= origin.x + size.x; x += step)
    {
        if (x < origin.x) continue;
        float worldX = off.x + (x - origin.x) / zoom;
        bool isYAxis = (std::abs(worldX) < 0.5f / zoom);
        ImU32 col = isYAxis ? IM_COL32(100, 180, 255, 200) : lineColor;
        float thickness = isYAxis ? 2.0f : 1.0f;
        dl->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + size.y), col, thickness);
    }

    // Horizontal lines
    for (float y = origin.y - phaseY; y <= origin.y + size.y; y += step)
    {
        if (y < origin.y) continue;
        float worldY = off.y + (y - origin.y) / zoom;
        bool isXAxis = (std::abs(worldY) < 0.5f / zoom);
        ImU32 col = isXAxis ? IM_COL32(255, 100, 100, 200) : lineColor;
        float thickness = isXAxis ? 2.0f : 1.0f;
        dl->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + size.x, y), col, thickness);
    }
}

void ScenePanel::drawSceneViewport()
{
    ImVec2 viewSize = ImGui::GetContentRegionAvail();
    int w = (int)viewSize.x;
    int h = (int)viewSize.y;
    if (w < 1 || h < 1) return;

    // Keep editor camera viewport in sync
    m_editorCamera.SetViewportSize(w, h);

    // Resize FBO if the panel size changed
    if (w != m_vpWidth || h != m_vpHeight)
        resizeFBO(w, h);

    ImGui::Image(
        (ImTextureID)(intptr_t)m_colorTex,
        viewSize,
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

    // Pan - right or middle drag
    if (ImGui::IsItemHovered())
    {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
            ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        {
            panEditorCamera(-delta.x, -delta.y);
        }

        // Zoom - scroll wheel
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
        {
            float oldZoom = m_editorCamera.GetZoom();
            float newZoom = std::max(0.1f, std::min(oldZoom * (1.0f + wheel * 0.1f), 10.0f));
            m_editorCamera.SetZoom(newZoom);
        }
    }

    // Zoom - scroll wheel
    if (ImGui::IsItemHovered())
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
        {
            float oldZoom = m_editorCamera.GetZoom();
            float newZoom = std::max(0.1f, std::min(oldZoom * (1.0f + wheel * 0.1f), 10.0f));
            m_editorCamera.SetZoom(newZoom);
        }
    }

    if (m_showGrid && m_vpWidth > 0 && m_vpHeight > 0)
        drawGrid();
}

void ScenePanel::panEditorCamera(float dx, float dy)
{
    float zoom = m_editorCamera.GetZoom();
    Vector2 current = m_editorCamera.GetOffset();
    m_editorCamera.SetOffset({ current.x + dx / zoom, current.y + dy / zoom });
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
    m_editorCamera.isMainCamera = true;
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
