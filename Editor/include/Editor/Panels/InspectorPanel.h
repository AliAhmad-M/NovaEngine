#pragma once

#include <Editor/Context.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Physics/Components.h>
#include <World/Components.h>
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

        void drawTransform2D(Transform2D& t);
        void drawCamera2D(Camera2D& c);
        void drawSprite2D(Sprite2D& s);
        void drawRigidBody2D(RigidBody2D& rb);
        void drawTilemap2D(Tilemap2D& tm);

        void drawAddComponentMenu();

        void drawTexturePathField(Sprite2D& s);

        Context* m_ctx = nullptr;
        ImGuiWindowFlags m_windowFlags = 0;
    };
}