#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <Editor/EditorContext.h>
#include <Graphics/Texture.h>
#include <imgui.h>

enum class EntityType { Entity, Camera, Light };

struct EntityNode
{
    int id = -1;
    std::string name = "Entity";
    EntityType  type = EntityType::Entity;
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
    GLuint GetEntityIcon(EntityType type) const;

    EditorContext*          m_ctx = nullptr;
    ImGuiWindowFlags        m_windowFlags = 0;
    std::vector<EntityNode> m_entities;

    int m_nextId = 0;

    Texture* m_iconEntity;
    Texture* m_iconCamera;
    Texture* m_iconLight;
};

