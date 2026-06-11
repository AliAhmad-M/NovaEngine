#include <Editor/Panels/ScenePanel.h>
#include <cstdio>
#include <cmath>
#include <algorithm>

ScenePanel::ScenePanel(EditorContext* ctx)
    : m_ctx(ctx)
{
    // Open a default untitled scene on startup
    NewScene();
}

void ScenePanel::Draw()
{
    ImGui::Begin("Scene");

    DrawSceneToolbar();
    ImGui::Separator();
    DrawTabBar();
    ImGui::Separator();
    DrawSceneViewport();

    ImGui::End();
}

void ScenePanel::DrawSceneToolbar()
{
    // Grid toggle
    ImGui::Checkbox("Grid", &m_showGrid);
    ImGui::SameLine();

    // New scene button
    if (ImGui::SmallButton("+ New Scene"))
        NewScene();
}

void ScenePanel::DrawTabBar()
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
            m_activeTab = i;
            // Sync context
            m_ctx->activeScenePath = tab.filePath.empty() ? tab.name : tab.filePath;
            ImGui::EndTabItem();
        }

        // Tab closed via the X button
        if (!open)
        {
            // TODO: prompt save scene
            CloseScene(i);
            break;
        }
    }

    ImGui::EndTabBar();
}

void ScenePanel::DrawSceneViewport()
{
    // Reserve the remaining space for the viewport area
    ImVec2 viewSize = ImGui::GetContentRegionAvail();
    if (viewSize.x < 1.0f || viewSize.y < 1.0f) return;

    // Placeholder grey canvas
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled(canvasPos,
        ImVec2(canvasPos.x + viewSize.x, canvasPos.y + viewSize.y),
        IM_COL32(30, 30, 35, 255));

    // Grid lines
    if (m_showGrid && m_gridSize > 0)
    {
        float scaledGrid = m_gridSize * m_viewportZoom;
        ImU32 gridColor = IM_COL32(55, 55, 65, 200);

        // Offset so grid pans with camera
        float ox = std::fmod(m_cameraOffset[0], scaledGrid);
        float oy = std::fmod(m_cameraOffset[1], scaledGrid);

        for (float x = canvasPos.x + ox; x < canvasPos.x + viewSize.x; x += scaledGrid)
            dl->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasPos.y + viewSize.y), gridColor);

        for (float y = canvasPos.y + oy; y < canvasPos.y + viewSize.y; y += scaledGrid)
            dl->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + viewSize.x, y), gridColor);

        // Origin cross
        float cx = canvasPos.x + m_cameraOffset[0];
        float cy = canvasPos.y + m_cameraOffset[1];
        dl->AddLine(ImVec2(cx, canvasPos.y), ImVec2(cx, canvasPos.y + viewSize.y),
            IM_COL32(80, 80, 100, 255));
        dl->AddLine(ImVec2(canvasPos.x, cy), ImVec2(canvasPos.x + viewSize.x, cy),
            IM_COL32(80, 80, 100, 255));
    }

    // "No scene content" hint
    ImVec2 center(canvasPos.x + viewSize.x * 0.5f, canvasPos.y + viewSize.y * 0.5f);
    const char* hint = "Scene Viewport  (connect renderer here)";
    ImVec2 textSize = ImGui::CalcTextSize(hint);
    dl->AddText(ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f),
        IM_COL32(70, 70, 80, 255), hint);

    // Invisible button to capture mouse input over the viewport
    ImGui::InvisibleButton("##viewport", viewSize,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    // Pan with middle-mouse or right-drag inside the viewport
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

    // Zoom with scroll wheel when hovered
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

void ScenePanel::NewScene()
{
    int idx = (int)m_sceneTabs.size();
    char name[64];
    std::snprintf(name, sizeof(name), "Untitled_%d", idx);
    m_sceneTabs.push_back({ name, "", false });
}

void ScenePanel::CloseScene(int index)
{
    if (index < 0 || index >= (int)m_sceneTabs.size()) return;
    m_sceneTabs.erase(m_sceneTabs.begin() + index);
    if (m_activeTab >= (int)m_sceneTabs.size())
        m_activeTab = (int)m_sceneTabs.size() - 1;
}
