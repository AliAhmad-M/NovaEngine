#pragma once
#pragma once
#include <Editor/Context.h>
#include <Platform/InputMap.h>
#include <imgui.h>
#include <string>
#include <vector>

namespace Nova::Editor
{
    class InputMapPanel
    {
    public:
        explicit InputMapPanel(EditorContext* ctx);
        void Draw(bool& open);
        void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

    private:
        void drawToolbar();
        void drawBindingList();
        void drawAddBindingPopup();
        void drawEditBindingPopup();
        void applyBindings();

        const char* glfwKeyName(int key) const;
        int         nameToGlfwKey(const char* name) const;

        EditorContext* m_ctx = nullptr;
        ImGuiWindowFlags m_windowFlags = 0;
        InputMap* m_inputMap = nullptr;

        struct BindingEntry
        {
            std::string action;
            int         glfwKey = GLFW_KEY_UNKNOWN;
            bool        isMouseButton = false;
        };

        std::vector<BindingEntry> m_bindings;
        int                       m_editingIndex = -1;
        bool                      m_listeningKey = false;
        char                      m_newAction[64] = {};
        int                       m_selectedIndex = -1;

        // Available keys list for dropdown
        struct KeyEntry { const char* name; int code; };
        static const KeyEntry s_keys[];
    };
}