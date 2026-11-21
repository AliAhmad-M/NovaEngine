#pragma once

#include <Core/ECS/ECS.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Graphics/Texture.h>
#include <Physics/Components.h>
#include <World/Components.h>
#include <Runtime/ContentLoader.h>
#include <Editor/Context.h>
#include <optional>
#include <imgui.h>

namespace Nova::Editor
{
    class InspectorPanel
    {
    public:
        explicit InspectorPanel(EditorContext* ctx);

        void Draw();
        void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

    private:
        struct PickerRequest
        {
            const char* popupId = nullptr;
            const char* filterExt = "";
            std::string initialPath = "";
            std::function<void(const std::string&)> onConfirm;

            PickerRequest() = default;
            PickerRequest(const char* id, const char* ext, std::function<void(const std::string&)> cb)
                : popupId(id), filterExt(ext), onConfirm(std::move(cb)) {}
            PickerRequest(const char* id, const char* ext, std::string initial, std::function<void(const std::string&)> cb)
                : popupId(id), filterExt(ext), initialPath(std::move(initial)), onConfirm(std::move(cb)) {}
        };
        bool m_pickerOpen = false;
        std::optional<PickerRequest> m_pendingPicker;

        void drawAddComponentMenu();
        void drawBehaviorScript(BehaviorScript& sc);
        void generateBehaviorStub(const std::string& path, const std::string& className);

        EditorContext* m_ctxRef;
        ImGuiWindowFlags m_windowFlags = 0;

        ECS* m_ecsRef;
        ContentLoader* m_contentLoaderRef;

        // Component icons
        Texture* m_iconTransform = nullptr;
        Texture* m_iconSprite = nullptr;
        Texture* m_iconCamera = nullptr;
        Texture* m_iconRigidbody = nullptr;
        Texture* m_iconTilemap = nullptr;
        Texture* m_iconScript = nullptr;
    };
}