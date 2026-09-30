#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Panels/HierarchyPanel.h>
#include <Editor/Panels/ScenePanel.h>
#include <Editor/Scene/Scene.h>
#include <Core/Services.h>
#include <imgui.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

using namespace Nova::Editor;

HierarchyPanel::HierarchyPanel(Context* ctx) : m_ctxRef(ctx)
{
    m_ecsRef = Services::Fetch<ECS>();
    m_contentLoaderRef = Services::Fetch<ContentLoader>();

    m_iconEntity = m_contentLoaderRef->LoadTexture("assets/icons/icon_entity.png");
    m_iconCamera = m_contentLoaderRef->LoadTexture("assets/icons/icon_camera.png");
    m_iconLight = m_contentLoaderRef->LoadTexture("assets/icons/icon_light.png");
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

void HierarchyPanel::createEntity(EntityID parentId)
{
    if (!m_ctxRef->activeSceneRef)
        return;

    EntityID newEntity = m_ecsRef->CreateEntity();

    char name[64];
    std::snprintf(name, sizeof(name), "Entity_%zu",
        m_ctxRef->activeSceneRef->GetEntities().size());

    m_ctxRef->activeSceneRef->AddEntity(newEntity, name, parentId);

    if (m_ctxRef->scenePanelRef)
        m_ctxRef->scenePanelRef->MarkSceneAsDirty();
}

void HierarchyPanel::drawCreateButton()
{
    if (!m_ctxRef->activeSceneRef) return;

    if (ImGui::Button("+ Add Entity"))
        createEntity(NO_PARENT);
}

void HierarchyPanel::drawEntityList()
{
    auto& entities = m_ctxRef->activeSceneRef->GetEntities();

    // Draw only root entities
    for (auto& node : entities)
    {
        if (node.parentId != NO_PARENT) continue;
        drawEntityNode(node, entities);
    }
}

void HierarchyPanel::drawEntityNode(SceneEntity& node,
    std::vector<SceneEntity>& entities)
{
    // Apply search filter
    if (m_searchBuffer[0] != '\0')
    {
        std::string name = node.name;
        std::string search = m_searchBuffer;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::transform(search.begin(), search.end(), search.begin(), ::tolower);
        if (name.find(search) == std::string::npos) return;
    }

    bool selected = (m_ctxRef->selectedEntity == static_cast<int>(node.id));
    auto children = m_ctxRef->activeSceneRef->GetChildren(node.id);
    bool hasChildren = !children.empty();

    ImGui::PushID(static_cast<int>(node.id));

    ImVec2 rowPos = ImGui::GetCursorPos();

    ImGuiTreeNodeFlags treeFlags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_AllowOverlap;

    if (selected)     treeFlags |= ImGuiTreeNodeFlags_Selected;
    if (!hasChildren) treeFlags |= ImGuiTreeNodeFlags_Leaf;

    // Invisible selectable as full row background
    ImGui::Selectable("##bg", selected,
        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick,
        ImVec2(0, 24));

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        m_ctxRef->selectedEntity = static_cast<int>(node.id);

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        m_renamingEntity = static_cast<int>(node.id);
        std::strncpy(m_renameBuffer, node.name.c_str(), sizeof(m_renameBuffer));
        m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        m_ctxRef->selectedEntity = static_cast<int>(node.id);
        ImGui::OpenPopup("EntityRowMenu");
    }

    // Drag source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        EntityID dragId = node.id;
        ImGui::SetDragDropPayload("ENTITY_ID", &dragId, sizeof(EntityID));
        ImGui::Text("Move: %s", node.name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drop target — drop onto entity to parent it
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload("ENTITY_ID"))
        {
            EntityID draggedId = *(const EntityID*)payload->Data;
            if (draggedId != node.id)
            {
                m_ctxRef->activeSceneRef->SetParent(draggedId, node.id);
                if (m_ctxRef->scenePanelRef)
                    m_ctxRef->scenePanelRef->MarkSceneAsDirty();
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Draw tree node arrow + icon + name on same row
    ImGui::SetCursorPos(rowPos);
    std::string hiddenLabel = "##node_" + std::to_string(node.id);
    bool open = ImGui::TreeNodeEx(hiddenLabel.c_str(), treeFlags);

    ImGui::SameLine();

    ImVec4 tint = selected
        ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
        : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

    if (m_iconEntity && m_iconEntity->GetHandle())
    {
        ImGui::Image(
            (ImTextureID)(intptr_t)m_iconEntity->GetHandle(),
            ImVec2(20, 20),
            ImVec2(0, 0),
            ImVec2(1, 1),
            tint,
            ImVec4(0, 0, 0, 0)
        );
        ImGui::SameLine();
    }

    ImGui::SetCursorPosY(rowPos.y + 4.0f);

    // Rename input or label
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

    // Context menu
    if (ImGui::BeginPopup("EntityRowMenu"))
    {
        ImGui::TextDisabled("%s", node.name.c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Rename"))
        {
            m_renamingEntity = static_cast<int>(node.id);
            std::strncpy(m_renameBuffer, node.name.c_str(), sizeof(m_renameBuffer));
            m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Create Child Entity"))
        {
            createEntity(node.id);
            ImGui::CloseCurrentPopup();
        }

        if (node.parentId != NO_PARENT && ImGui::MenuItem("Unparent"))
        {
            m_ctxRef->activeSceneRef->SetParent(node.id, NO_PARENT);
            if (m_ctxRef->scenePanelRef)
                m_ctxRef->scenePanelRef->MarkSceneAsDirty();
            ImGui::CloseCurrentPopup();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete"))
        {
            EntityID idToRemove = node.id;
            m_ecsRef->DeleteEntity(idToRemove);
            m_ctxRef->activeSceneRef->RemoveEntity(idToRemove);
            if (m_ctxRef->selectedEntity == static_cast<int>(idToRemove))
                m_ctxRef->selectedEntity = -1;
            if (m_ctxRef->scenePanelRef)
                m_ctxRef->scenePanelRef->MarkSceneAsDirty();
            ImGui::EndPopup();
            if (open) ImGui::TreePop();
            ImGui::PopID();
            return;
        }

        ImGui::EndPopup();
    }

    // Draw children recursively
    if (open)
    {
        for (auto& child : entities)
            if (child.parentId == node.id)
                drawEntityNode(child, entities);
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void HierarchyPanel::drawContextMenu()
{
    if (ImGui::BeginPopup("HierarchyContextMenu"))
    {
        if (ImGui::MenuItem("Create Empty Entity"))
            createEntity(NO_PARENT);
        ImGui::EndPopup();
    }
}