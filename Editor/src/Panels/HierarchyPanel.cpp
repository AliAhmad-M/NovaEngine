#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Panels/HierarchyPanel.h>
#include <imgui.h>
#include <cstring>
#include <cstdio>

HierarchyPanel::HierarchyPanel(EditorContext* ctx) : m_ctx(ctx)
{
    // Load icons
    m_iconEntity = ctx->contentLoader.LoadTexture("assets/icons/icon_entity.png");
    m_iconCamera = ctx->contentLoader.LoadTexture("assets/icons/icon_camera.png");
    m_iconLight = ctx->contentLoader.LoadTexture("assets/icons/icon_light.png");

    // Placeholder entities
    m_entities.push_back({ m_nextId++, "Camera", EntityType::Camera });
    m_entities.push_back({ m_nextId++, "Entity0", EntityType::Entity });
    m_entities.push_back({ m_nextId++, "Entity1", EntityType::Entity });
}

GLuint HierarchyPanel::GetEntityIcon(EntityType type) const
{
    switch (type)
    {
        case EntityType::Camera:  return m_iconCamera->GetHandle();
        case EntityType::Light:   return m_iconLight->GetHandle();
        default:                  return m_iconEntity->GetHandle();
    }
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
        m_entities.push_back({ m_nextId++, name, EntityType::Entity });
    }
    }
void HierarchyPanel::DrawEntityList()
{
    for (auto& node : m_entities)
    {
        bool selected = (m_ctx->selectedEntity == node.id);

        ImGui::PushID(node.id);

        // Save cursor position
        ImVec2 rowPos = ImGui::GetCursorPos();

        // Draw selectable background first (invisible label)
        if (ImGui::Selectable("##row", selected,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick,
            ImVec2(0, 24)))
        {
            m_ctx->selectedEntity = node.id;
        }

        // Right-click
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_ctx->selectedEntity = node.id;
            ImGui::OpenPopup("EntityRowMenu");
        }

        // Draw icon and text
        ImGui::SetCursorPos(rowPos);

        // Icon tint
        ImVec4 tint = selected
            ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
            : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

        GLuint iconTex = GetEntityIcon(node.type);
        ImGui::Image(
            (ImTextureID)(intptr_t)iconTex,
            ImVec2(30, 30),
            ImVec2(0, 0),
            ImVec2(1, 1),
            tint,
            ImVec4(0, 0, 0, 0)
        );
        ImGui::SameLine();
        ImGui::SetCursorPosY(rowPos.y + 4.0f);  // vertically center text
        ImGui::Text("%s", node.name.c_str());

        // Per-row context menu
        if (ImGui::BeginPopup("EntityRowMenu"))
        {
            ImGui::TextDisabled("%s", node.name.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
            {
                m_entities.erase(
                    std::remove_if(m_entities.begin(), m_entities.end(),
                        [&](const EntityNode& e) { return e.id == node.id; }),
                    m_entities.end());
                if (m_ctx->selectedEntity == node.id)
                    m_ctx->selectedEntity = -1;
                ImGui::EndPopup();
                ImGui::PopID();
                break;
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
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
