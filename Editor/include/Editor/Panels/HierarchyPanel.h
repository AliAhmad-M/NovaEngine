#pragma once
#include <string>
#include <Editor/EditorContext.h>
#include <Graphics/Texture.h>
#include <imgui.h>

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
    void CreateEntity();

    EditorContext* m_ctx = nullptr;
    ImGuiWindowFlags m_windowFlags = 0;

    Texture* m_iconEntity;
    Texture* m_iconCamera;
    Texture* m_iconLight;
};