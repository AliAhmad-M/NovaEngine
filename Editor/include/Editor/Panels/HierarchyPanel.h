#pragma once
#include <Core/ECS/ECS.h>
#include <Editor/Context.h>
#include <Graphics/Texture.h>
#include <Runtime/ContentLoader.h>
#include <string>
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
        int  m_renamingEntity = -1;
        char m_renameBuffer[128] = {};
        char m_searchBuffer[128] = {};
        void drawEntityList();
        void drawContextMenu();
        void drawCreateButton();
        void createEntity();
        void drawSearchBar();

        Context* m_ctxRef = nullptr;
        ImGuiWindowFlags m_windowFlags = 0;

        Texture* m_iconEntity;
        Texture* m_iconCamera;
        Texture* m_iconLight;

        ECS* m_ecsRef;
        ContentLoader* m_contentLoaderRef;
    };
}
