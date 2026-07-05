#pragma once

#include <Editor/Context.h>
#include <Core/ECS/ECS.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Graphics/Texture.h>
#include <Physics/Components.h>
#include <World/Components.h>
#include <Runtime/ContentLoader.h>
#include <optional>
#include <imgui.h>

namespace Nova::Editor
{
    class InspectorPanel
    {
    public:
        explicit InspectorPanel(Context* ctx);

        void Draw();
        void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

    private:
        struct PickerRequest
        {
            const char* popupId = nullptr;
            const char* filterExt = "";
            std::function<void(const std::string&)> onConfirm;
        };
        bool m_pickerOpen = false;
        std::optional<PickerRequest> m_pendingPicker;

        void drawAddComponentMenu();

        void drawTransform2D(Transform2D& t);
        void drawCamera2D(Camera2D& c);
        void drawSprite2D(Sprite2D& s);
        void drawTexturePathField(Sprite2D& s);
        void drawRigidBody2D(RigidBody2D& rb);
        void drawTilemap2D(Tilemap2D& tm);
        void drawBehaviorScript(BehaviorScript& sc);
        void generateBehaviorStub(const std::string& path, const std::string& className);

        Context* m_ctx;
        ImGuiWindowFlags m_windowFlags = 0;

        ECS* m_ecs;
        ContentLoader* m_contentLoader;

        // Component icons
        Texture* m_iconTransform = nullptr;
        Texture* m_iconSprite = nullptr;
        Texture* m_iconCamera = nullptr;
        Texture* m_iconRigidbody = nullptr;
        Texture* m_iconTilemap = nullptr;
        Texture* m_iconScript = nullptr;
    };
}