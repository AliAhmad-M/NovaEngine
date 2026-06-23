#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Panels/HierarchyPanel.h>
#include <Editor/Panels/ScenePanel.h>
#include <Core/Services.h>
#include <imgui.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

using namespace Nova::Editor;

HierarchyPanel::HierarchyPanel(Context* ctx) : m_ctx(ctx)
{
    m_ecs = Services::Fetch<ECS>(); m_contentLoader = Services::Fetch<ContentLoader>();
    
    m_iconEntity = m_contentLoader->LoadTexture("assets/icons/icon_entity.png");
    m_iconCamera = m_contentLoader->LoadTexture("assets/icons/icon_camera.png");
    m_iconLight  = m_contentLoader->LoadTexture("assets/icons/icon_light.png");
}

void HierarchyPanel::Draw()
{
    ImGui::Begin("Hierarchy");

    drawCreateButton();
    ImGui::Separator();

    if (m_ctx->activeScene)
        drawEntityList();
    else
        ImGui::TextDisabled("No scene open");

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("HierarchyContextMenu");

    drawContextMenu();

    ImGui::End();
}

void HierarchyPanel::createEntity()
{
    if (!m_ctx->activeScene) return;

    EntityID newEntity = m_ecs->CreateEntity();

    char name[64];
    std::snprintf(name, sizeof(name), "Entity_%zu",
        m_ctx->activeScene->GetEntities().size());

    m_ctx->activeScene->AddEntity(newEntity, name);

    // Mark scene as modified
    if (m_ctx->scenePanel)
        m_ctx->scenePanel->MarkSceneAsDirty();
}

void HierarchyPanel::drawCreateButton()
{
    if (!m_ctx->activeScene) return;

    if (ImGui::Button("+ Add Entity"))
        createEntity();
}

void HierarchyPanel::drawEntityList()
{
    auto& entities = m_ctx->activeScene->GetEntities();

    for (auto& node : entities)
    {
        bool selected = (m_ctx->selectedEntity == static_cast<int>(node.id));

        ImGui::PushID(static_cast<int>(node.id));

        ImVec2 rowPos = ImGui::GetCursorPos();

        if (ImGui::Selectable("##row", selected,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick,
            ImVec2(0, 24)))
        {
            m_ctx->selectedEntity = static_cast<int>(node.id);
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_ctx->selectedEntity = static_cast<int>(node.id);
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
        ImGui::Text("%s", node.name.c_str());

        if (ImGui::BeginPopup("EntityRowMenu"))
        {
            ImGui::TextDisabled("%s", node.name.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
            {
                EntityID idToRemove = node.id;

                m_ecs->DeleteEntity(idToRemove);

                m_ctx->activeScene->RemoveEntity(idToRemove);

                if (m_ctx->selectedEntity == static_cast<int>(idToRemove))
                    m_ctx->selectedEntity = -1;

                // Mark scene as modified
                if (m_ctx->scenePanel)
                    m_ctx->scenePanel->MarkSceneAsDirty();

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