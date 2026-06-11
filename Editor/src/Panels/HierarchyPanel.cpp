#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Panels/HierarchyPanel.h>
#include <imgui.h>
#include <cstring>
#include <cstdio>

HierarchyPanel::HierarchyPanel(EditorContext* ctx) : m_ctx(ctx)
{
    // Placeholder entities
    m_entities.push_back({ m_nextId++, "Camera" });
    m_entities.push_back({ m_nextId++, "Entity0" });
    m_entities.push_back({ m_nextId++, "Entity1" });
}

void HierarchyPanel::Draw()
{
    ImGui::Begin("Hierarchy");

    DrawCreateButton();
    ImGui::Separator();
    DrawEntityList();

    // Right-click on empty space inside the panel
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("HierarchyContextMenu");

    DrawContextMenu();

    ImGui::End();
}

void HierarchyPanel::DrawCreateButton()
{
    if (ImGui::Button("+ Add Entity"))
    {
        char name[64];
        std::snprintf(name, sizeof(name), "Entity_%d", m_nextId);
        m_entities.push_back({ m_nextId++, name });
    }
}

void HierarchyPanel::DrawEntityList()
{
    static char filterBuf[128] = {};

    for (auto& node : m_entities)
    {
        // Selectable row
        bool selected = (m_ctx->selectedEntity == node.id);

        ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns |
            ImGuiSelectableFlags_AllowDoubleClick;
        if (ImGui::Selectable(node.name.c_str(), selected, flags))
        {
            m_ctx->selectedEntity = node.id;
        }

        // Right-click on a specific entity
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_ctx->selectedEntity = node.id;
            ImGui::OpenPopup("EntityRowMenu");
        }

        // Per-row context menu
        if (ImGui::BeginPopup("EntityRowMenu"))
        {
            ImGui::TextDisabled("%s", node.name.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
            {
                // Remove entity from list and deselect
                m_entities.erase(
                    std::remove_if(m_entities.begin(), m_entities.end(),
                        [&](const EntityNode& e) { return e.id == node.id; }),
                    m_entities.end());
                if (m_ctx->selectedEntity == node.id)
                    m_ctx->selectedEntity = -1;
                ImGui::EndPopup();
                break;
            }
            ImGui::EndPopup();
        }
    }
}

void HierarchyPanel::DrawContextMenu()
{
    if (ImGui::BeginPopup("HierarchyContextMenu"))
    {
        if (ImGui::MenuItem("Create Empty Entity"))
        {
            char name[64];
            std::snprintf(name, sizeof(name), "Entity_%d", m_nextId);
            m_entities.push_back({ m_nextId++, name });
        }
        ImGui::EndPopup();
    }
}
