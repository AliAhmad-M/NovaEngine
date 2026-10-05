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

    processPending();

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

void HierarchyPanel::drawEntityNode(SceneEntity& node, std::vector<SceneEntity>& entities)
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

    constexpr float rowH = 24.0f;
    constexpr float iconSize = 24.0f;
    constexpr float iconGap = 6.0f;

    bool selected = (m_ctxRef->selectedEntity == static_cast<int>(node.id));
    auto children = m_ctxRef->activeSceneRef->GetChildren(node.id);
    bool hasChildren = !children.empty();

    ImGui::PushID(static_cast<int>(node.id));

    ImGuiTreeNodeFlags treeFlags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_FramePadding |
        ImGuiTreeNodeFlags_AllowOverlap;

    if (selected)     treeFlags |= ImGuiTreeNodeFlags_Selected;
    if (!hasChildren) treeFlags |= ImGuiTreeNodeFlags_Leaf;

    // Pad vertically so the node is exactly rowH tall
    float padY = (rowH - ImGui::GetFontSize()) * 0.5f;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
        ImVec2(ImGui::GetStyle().FramePadding.x, padY));
    bool open = ImGui::TreeNodeEx("##node", treeFlags);
    ImGui::PopStyleVar();

    // Grab the row rect immediately
    ImVec2 rMin = ImGui::GetItemRectMin();
    ImVec2 rMax = ImGui::GetItemRectMax();

    // Selection
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
        m_ctxRef->selectedEntity = static_cast<int>(node.id);

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        m_renamingEntity = static_cast<int>(node.id);
        std::strncpy(m_renameBuffer, node.name.c_str(), sizeof(m_renameBuffer));
        m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        m_ctxRef->selectedEntity = static_cast<int>(node.id);

    // Drag source
    if (ImGui::BeginDragDropSource())
    {
        EntityID dragId = node.id;
        ImGui::SetDragDropPayload("ENTITY_ID", &dragId, sizeof(EntityID));
        ImGui::Text("Move: %s", node.name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drop target
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID"))
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

    // Context menu
    if (ImGui::BeginPopupContextItem("EntityRowMenu"))
    {
        ImGui::TextDisabled("%s", node.name.c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Rename"))
        {
            m_renamingEntity = static_cast<int>(node.id);
            std::strncpy(m_renameBuffer, node.name.c_str(), sizeof(m_renameBuffer));
            m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
        }

        if (ImGui::MenuItem("Create Child Entity"))
        {
            m_hasPendingCreate = true;
            m_pendingCreateParent = node.id;
        }

        if (node.parentId != NO_PARENT && ImGui::MenuItem("Unparent"))
        {
            m_ctxRef->activeSceneRef->SetParent(node.id, NO_PARENT);
            if (m_ctxRef->scenePanelRef)
                m_ctxRef->scenePanelRef->MarkSceneAsDirty();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete"))
            m_pendingDelete = static_cast<int>(node.id);

        ImGui::EndPopup();
    }

    // Icon + label, centered on the row
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float cy = (rMin.y + rMax.y) * 0.5f;
    float x = rMin.x + ImGui::GetTreeNodeToLabelSpacing();

    if (m_iconEntity && m_iconEntity->GetHandle())
    {
        ImU32 tint = selected ? IM_COL32(255, 255, 255, 255)
            : IM_COL32(153, 153, 153, 255);
        dl->AddImage(
            (ImTextureID)(intptr_t)m_iconEntity->GetHandle(),
            ImVec2(x, cy - iconSize * 0.5f),
            ImVec2(x + iconSize, cy + iconSize * 0.5f),
            ImVec2(0, 0), ImVec2(1, 1), tint);
        x += iconSize + iconGap;
    }

    if (m_renamingEntity == static_cast<int>(node.id))
    {
        ImVec2 next = ImGui::GetCursorScreenPos();   // where the next row starts
        ImGui::SetCursorScreenPos(ImVec2(x, cy - ImGui::GetFrameHeight() * 0.5f));

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

        ImGui::SetCursorScreenPos(next);
    }
    else
    {
        dl->AddText(ImVec2(x, cy - ImGui::GetFontSize() * 0.5f),
            ImGui::GetColorU32(ImGuiCol_Text), node.name.c_str());
    }

    // Children
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

void HierarchyPanel::processPending()
{
    if (m_pendingDelete != -1)
    {
        deleteEntityRecursive(static_cast<EntityID>(m_pendingDelete));
        m_pendingDelete = -1;

        if (m_ctxRef->scenePanelRef)
            m_ctxRef->scenePanelRef->MarkSceneAsDirty();
    }

    if (m_hasPendingCreate)
    {
        createEntity(m_pendingCreateParent);
        m_hasPendingCreate = false;
    }
}

void HierarchyPanel::deleteEntityRecursive(EntityID id)
{
    auto& entities = m_ctxRef->activeSceneRef->GetEntities();

    std::vector<EntityID> children;
    for (auto& e : entities)
        if (e.parentId == id)
            children.push_back(e.id);

    for (EntityID c : children)
        deleteEntityRecursive(c);

    m_ecsRef->DeleteEntity(id);
    m_ctxRef->activeSceneRef->RemoveEntity(id);

    if (m_ctxRef->selectedEntity == static_cast<int>(id))
        m_ctxRef->selectedEntity = -1;
}