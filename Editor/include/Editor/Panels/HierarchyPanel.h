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
        explicit HierarchyPanel(EditorContext* ctx);

        void Draw();
        void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

    private:
        int  m_renamingEntity = -1;
        char m_renameBuffer[128] = {};
        char m_searchBuffer[128] = {};
        EntityID m_pendingCreateParent = NO_PARENT;


        void drawEntityList();
        void drawContextMenu();
        void drawCreateButton();
        void createEntity(EntityID parentId = NO_PARENT);
        void drawEntityNode(SceneEntity& node, std::vector<SceneEntity>& entities);
        void drawSearchBar();
        void processPending();
        void deleteEntityRecursive(EntityID id);

        enum class DropZone { Into, Before, After };
        enum class PendingType { None, Create, Delete, Move };

        struct PendingAction
        {
            PendingType type = PendingType::None;
            EntityID    id = 0;
            EntityID    target = 0;             // Move only
            DropZone    zone = DropZone::Into;  // Move only
        };

        PendingAction m_pending;

        bool wouldCreateCycle(EntityID id, EntityID newParent);
        int  findIndex(EntityID id);
        bool findSibling(EntityID id, int dir, EntityID& out);
        bool applyMove(const PendingAction& m);

        EditorContext* m_ctxRef = nullptr;
        ImGuiWindowFlags m_windowFlags = 0;

        Texture* m_iconEntity;
        Texture* m_iconCamera;
        Texture* m_iconLight;

        ECS* m_ecsRef;
        ContentLoader* m_contentLoaderRef;
    };
}
