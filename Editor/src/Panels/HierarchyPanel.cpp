#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Panels/HierarchyPanel.h>
#include <Editor/Panels/ScenePanel.h>
#include <Core/Services.h>
#include <imgui.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

using namespace Nova::Editor;

HierarchyPanel::HierarchyPanel(Context* ctx) : m_ctxRef(ctx)
{
    m_ecsRef = Services::Fetch<ECS>(); m_contentLoaderRef = Services::Fetch<ContentLoader>();
    
    m_iconEntity = m_contentLoaderRef->LoadTexture("assets/icons/icon_entity.png");
    m_iconCamera = m_contentLoaderRef->LoadTexture("assets/icons/icon_camera.png");
    m_iconLight  = m_contentLoaderRef->LoadTexture("assets/icons/icon_light.png");
}

void HierarchyPanel::drawSearchBar()
{
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##search", "Search entities...",
        m_searchBuffer, sizeof(m_searchBuffer));
}

void HierarchyPanel::Draw()
{
    ImGui::Begin("Hierarchy");

    drawCreateButton();
    ImGui::Spacing();
    drawSearchBar();
    ImGui::Separator();

    if (m_ctxRef->activeSceneRef)
        drawEntityList();
    else
        ImGui::TextDisabled("No scene open");

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)
        && m_ctxRef->selectedEntity == -1)
        ImGui::OpenPopup("HierarchyContextMenu");

    drawContextMenu();

    ImGui::End();
}

void HierarchyPanel::createEntity()
{
    if (!m_ctxRef->activeSceneRef) 
    {
        printf("[DEBUG]: No active scene\n");
        return;
    }

    EntityID newEntity = m_ecsRef->CreateEntity();
    printf("[DEBUG]: Created entity ID: %d\n", (int)newEntity);

    char name[64];
    std::snprintf(name, sizeof(name), "Entity_%zu",
        m_ctxRef->activeSceneRef->GetEntities().size());

    m_ctxRef->activeSceneRef->AddEntity(newEntity, name);
    printf("[DEBUG]: Added entity '%s' to scene, total entities: %zu\n", 
        name, m_ctxRef->activeSceneRef->GetEntities().size());

    if (m_ctxRef->scenePanelRef)
        m_ctxRef->scenePanelRef->MarkSceneAsDirty();
}

void HierarchyPanel::drawCreateButton()
{
    if (!m_ctxRef->activeSceneRef) return;

    if (ImGui::Button("+ Add Entity"))
        createEntity();
}

void HierarchyPanel::drawEntityList()
{
    auto& entities = m_ctxRef->activeSceneRef->GetEntities();

    for (auto& node : entities)
    {
        if (m_searchBuffer[0] != '\0')
        {
            std::string name = node.name;
            std::string search = m_searchBuffer;

            // Case insensitive search
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            std::transform(search.begin(), search.end(), search.begin(), ::tolower);

            if (name.find(search) == std::string::npos)
                continue;  // skip entities that don't match
        }

        bool selected = (m_ctxRef->selectedEntity == static_cast<int>(node.id));

        ImGui::PushID(static_cast<int>(node.id));

        ImVec2 rowPos = ImGui::GetCursorPos();

        if (ImGui::Selectable("##row", selected,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick,
            ImVec2(0, 24)))
        {
            m_ctxRef->selectedEntity = static_cast<int>(node.id);
        }

        // Check double click separately on hovered item
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            m_renamingEntity = static_cast<int>(node.id);
            std::strncpy(m_renameBuffer, node.name.c_str(), sizeof(m_renameBuffer));
            m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
        }

        // Right click
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_ctxRef->selectedEntity = static_cast<int>(node.id);
            ImGui::OpenPopup("EntityRowMenu");
        }

        ImGui::SetCursorPos(rowPos);

        ImVec4 tint = selected
            ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
            : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

        GLuint iconTex = m_iconEntity->GetHandle();
        ImGui::Image(
            (ImTextureID)(intptr_t)iconTex,
            ImVec2(30, 30),
            ImVec2(0, 0),
            ImVec2(1, 1),
            tint,
            ImVec4(0, 0, 0, 0)
        );
        ImGui::SameLine();
        ImGui::SetCursorPosY(rowPos.y + 4.0f);

        // Show rename input or normal text
        if (m_renamingEntity == static_cast<int>(node.id))
        {
            ImGui::SetNextItemWidth(150.0f);
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                node.name = m_renameBuffer;
                m_renamingEntity = -1;
                if (m_ctxRef->scenePanelRef)
                    m_ctxRef->scenePanelRef->MarkSceneAsDirty();

                // Auto save after rename
                if (m_ctxRef->onSaveScene)
                    m_ctxRef->onSaveScene();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                m_renamingEntity = -1;
        }
        else
        {
            ImGui::Text("%s", node.name.c_str());
        }
        

        if (ImGui::BeginPopup("EntityRowMenu"))
        {
            ImGui::TextDisabled("%s", node.name.c_str());
            ImGui::Separator();

            // Rename option
            if (ImGui::MenuItem("Rename"))
            {
                m_renamingEntity = static_cast<int>(node.id);
                std::strncpy(m_renameBuffer, node.name.c_str(), sizeof(m_renameBuffer));
                m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
                ImGui::CloseCurrentPopup();
            }

            // Delete option
            if (ImGui::MenuItem("Delete"))
            {
                EntityID idToRemove = node.id;

                m_ecsRef->DeleteEntity(idToRemove);

                m_ctxRef->activeSceneRef->RemoveEntity(idToRemove);

                if (m_ctxRef->selectedEntity == static_cast<int>(idToRemove))
                    m_ctxRef->selectedEntity = -1;

                // Mark scene as modified
                if (m_ctxRef->scenePanelRef)
                    m_ctxRef->scenePanelRef->MarkSceneAsDirty();

                ImGui::EndPopup();
                ImGui::PopID();
                break;
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
}

void HierarchyPanel::drawContextMenu()
{
    if (ImGui::BeginPopup("HierarchyContextMenu"))
    {
        if (ImGui::MenuItem("Create Empty Entity"))
        {
            createEntity();
        }
        ImGui::EndPopup();
    }
}