#pragma once
#include <string>
#include <Editor/Context.h>
#include <Graphics/Texture.h>
#include <imgui.h>

namespace Nova::Editor 
{
    class HierarchyPanel
    {
    public:
        explicit HierarchyPanel(Context* ctx);

        void Draw();
        void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

    private:
        void drawEntityList();
        void drawContextMenu();
        void drawCreateButton();
        void createEntity();

        Context* m_ctx = nullptr;
        ImGuiWindowFlags m_windowFlags = 0;

        Texture* m_iconEntity;
        Texture* m_iconCamera;
        Texture* m_iconLight;
    };
}
