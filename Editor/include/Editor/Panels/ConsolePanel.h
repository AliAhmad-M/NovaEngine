#pragma once
#include <imgui.h>
#include <Editor/Context.h>

namespace Nova::Editor
{
    class ConsolePanel
    {
    public:
        explicit ConsolePanel(Context* ctx);
        void Draw();
        void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

    private:
        Context* m_ctx = nullptr;
        ImGuiWindowFlags m_windowFlags = 0;

        bool m_scrollToBottom = true;
        bool m_showLog = true;
        bool m_showInfo = true;
        bool m_showWarn = true;
        bool m_showError = true;

        bool   m_collapse = false;
        bool   m_autoScroll = true;
        int    m_selected = -1;
        size_t m_lastCount = 0;

        char m_filter[128] = {};
    };
}
