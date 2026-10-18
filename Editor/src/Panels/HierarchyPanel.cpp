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

HierarchyPanel::HierarchyPanel(EditorContext* ctx) : m_ctxRef(ctx)
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
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_ID",
            ImGuiDragDropFlags_AcceptBeforeDelivery |
            ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

        if (payload)
        {
            EntityID draggedId = *(const EntityID*)payload->Data;

            float t = (ImGui::GetMousePos().y - rMin.y) / (rMax.y - rMin.y);
            DropZone zone = DropZone::Into;
            if (t < 0.25f)
                zone = DropZone::Before;
            else if (t > 0.75f && !(open && hasChildren)) 
                zone = DropZone::After;

            EntityID newParent = (zone == DropZone::Into) ? node.id : node.parentId;
            bool valid = draggedId != node.id && !wouldCreateCycle(draggedId, newParent);

            ImU32 col = valid ? IM_COL32(66, 150, 255, 255) : IM_COL32(255, 80, 80, 255);
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            if (zone == DropZone::Into)
                fg->AddRect(rMin, rMax, col, 4.0f, 0, 2.0f);
            else
            {
                float y = (zone == DropZone::Before) ? rMin.y : rMax.y;
                fg->AddLine(ImVec2(rMin.x, y), ImVec2(rMax.x, y), col, 2.0f);
            }

            if (valid && payload->IsDelivery())
                m_pending = { PendingType::Move, draggedId, node.id, zone };
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
            m_pending = { PendingType::Create, node.id };
            m_pendingCreateParent = node.id;
        }

        if (node.parentId != NO_PARENT && ImGui::MenuItem("Unparent"))
        {
            m_ctxRef->activeSceneRef->SetParent(node.id, NO_PARENT);
            if (m_ctxRef->scenePanelRef)
                m_ctxRef->scenePanelRef->MarkSceneAsDirty();
        }

        EntityID sibling;
        if (ImGui::MenuItem("Move Up", nullptr, false, findSibling(node.id, -1, sibling)))
            m_pending = { PendingType::Move, node.id, sibling, DropZone::Before };

        if (ImGui::MenuItem("Move Down", nullptr, false, findSibling(node.id, +1, sibling)))
            m_pending = { PendingType::Move, node.id, sibling, DropZone::After };

        ImGui::Separator();

        if (ImGui::MenuItem("Delete"))
            m_pending = { PendingType::Delete, node.id };

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
    if (m_pending.type == PendingType::None) return;

    PendingAction a = m_pending;
    m_pending = {};

    bool changed = false;

    switch (a.type)
    {
    case PendingType::Delete:
        deleteEntityRecursive(a.id);
        changed = true;
        break;

    case PendingType::Create:
        createEntity(a.id);
        break;

    case PendingType::Move:
        changed = applyMove(a);
        break;

    default:
        break;
    }

    if (changed && m_ctxRef->scenePanelRef)
        m_ctxRef->scenePanelRef->MarkSceneAsDirty();
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

int HierarchyPanel::findIndex(EntityID id)
{
    auto& entities = m_ctxRef->activeSceneRef->GetEntities();
    for (int i = 0; i < (int)entities.size(); ++i)
        if (entities[i].id == id) return i;
    return -1;
}

bool HierarchyPanel::wouldCreateCycle(EntityID id, EntityID newParent)
{
    if (newParent == NO_PARENT) return false;
    if (newParent == id) return true;

    auto& entities = m_ctxRef->activeSceneRef->GetEntities();
    EntityID cur = newParent;

    for (size_t steps = 0; steps <= entities.size(); ++steps)
    {
        if (cur == NO_PARENT) return false;
        if (cur == id) return true;

        int idx = findIndex(cur);
        if (idx < 0) return false;
        cur = entities[idx].parentId;
    }
    return true;
}

bool HierarchyPanel::findSibling(EntityID id, int dir, EntityID& out)
{
    auto& entities = m_ctxRef->activeSceneRef->GetEntities();
    int idx = findIndex(id);
    if (idx < 0) return false;

    EntityID parent = entities[idx].parentId;
    for (int i = idx + dir; i >= 0 && i < (int)entities.size(); i += dir)
    {
        if (entities[i].parentId == parent)
        {
            out = entities[i].id;
            return true;
        }
    }
    return false;
}

bool HierarchyPanel::applyMove(const PendingAction& m)
{
    Scene* scene = m_ctxRef->activeSceneRef;
    if (!scene || m.id == m.target) return false;

    if (m.zone == DropZone::Into)
    {
        if (wouldCreateCycle(m.id, m.target)) return false;
        scene->SetParent(m.id, m.target);
        return true;
    }

    auto& entities = scene->GetEntities();
    int targetIdx = findIndex(m.target);
    if (targetIdx < 0 || findIndex(m.id) < 0) return false;

    EntityID newParent = entities[targetIdx].parentId;
    if (wouldCreateCycle(m.id, newParent)) return false;

    // Fix the order
    if (entities[findIndex(m.id)].parentId != newParent)
        scene->SetParent(m.id, newParent);

    int from = findIndex(m.id);
    SceneEntity moved = std::move(entities[from]);
    entities.erase(entities.begin() + from);

    int to = findIndex(m.target);   // re-find
    entities.insert(entities.begin() + (m.zone == DropZone::After ? to + 1 : to),
        std::move(moved));
    return true;
}