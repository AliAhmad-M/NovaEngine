#pragma once
#include <string>
#include <vector>
#include <Editor/EditorContext.h>
#include <imgui.h>

struct EntityNode
{
    int id = -1;
    std::string name = "Entity";
};

class HierarchyPanel
{
public:
    explicit HierarchyPanel(EditorContext* ctx);

    void Draw();
    void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

private:
    void DrawEntityList();
    void DrawContextMenu();
    void DrawCreateButton();

    EditorContext*          m_ctx = nullptr;
    ImGuiWindowFlags        m_windowFlags = 0;
    std::vector<EntityNode> m_entities;

    int m_nextId = 0;
};

