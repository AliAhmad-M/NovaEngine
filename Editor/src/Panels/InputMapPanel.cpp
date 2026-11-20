#define _CRT_SECURE_NO_WARNINGS
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>
#include <Editor/Panels/InputMapPanel.h>
#include <Core/Services.h>
#include <imgui.h>

#include <array>
#include <cstring>
#include <cstdio>

using namespace Nova::Editor;

// All GLFW keyboard keys and mouse buttons.
const InputMapPanel::KeyEntry InputMapPanel::s_keys[] =
{
    // Letters
    { "A", GLFW_KEY_A }, { "B", GLFW_KEY_B },
    { "C", GLFW_KEY_C }, { "D", GLFW_KEY_D },
    { "E", GLFW_KEY_E }, { "F", GLFW_KEY_F },
    { "G", GLFW_KEY_G }, { "H", GLFW_KEY_H },
    { "I", GLFW_KEY_I }, { "J", GLFW_KEY_J },
    { "K", GLFW_KEY_K }, { "L", GLFW_KEY_L },
    { "M", GLFW_KEY_M }, { "N", GLFW_KEY_N },
    { "O", GLFW_KEY_O }, { "P", GLFW_KEY_P },
    { "Q", GLFW_KEY_Q }, { "R", GLFW_KEY_R },
    { "S", GLFW_KEY_S }, { "T", GLFW_KEY_T },
    { "U", GLFW_KEY_U }, { "V", GLFW_KEY_V },
    { "W", GLFW_KEY_W }, { "X", GLFW_KEY_X },
    { "Y", GLFW_KEY_Y }, { "Z", GLFW_KEY_Z },

    // Numbers
    { "0", GLFW_KEY_0 }, { "1", GLFW_KEY_1 },
    { "2", GLFW_KEY_2 }, { "3", GLFW_KEY_3 },
    { "4", GLFW_KEY_4 }, { "5", GLFW_KEY_5 },
    { "6", GLFW_KEY_6 }, { "7", GLFW_KEY_7 },
    { "8", GLFW_KEY_8 }, { "9", GLFW_KEY_9 },

    // Printable keys
    { "Space",       GLFW_KEY_SPACE },
    { "Apostrophe",  GLFW_KEY_APOSTROPHE },
    { "Comma",       GLFW_KEY_COMMA },
    { "Minus",       GLFW_KEY_MINUS },
    { "Period",      GLFW_KEY_PERIOD },
    { "Slash",       GLFW_KEY_SLASH },
    { "Semicolon",   GLFW_KEY_SEMICOLON },
    { "Equal",       GLFW_KEY_EQUAL },
    { "Left Bracket",  GLFW_KEY_LEFT_BRACKET },
    { "Backslash",     GLFW_KEY_BACKSLASH },
    { "Right Bracket", GLFW_KEY_RIGHT_BRACKET },
    { "Grave Accent",  GLFW_KEY_GRAVE_ACCENT },

    // International keys
    { "World 1", GLFW_KEY_WORLD_1 },
    { "World 2", GLFW_KEY_WORLD_2 },

    // Navigation and control
    { "Escape",    GLFW_KEY_ESCAPE },
    { "Enter",     GLFW_KEY_ENTER },
    { "Tab",       GLFW_KEY_TAB },
    { "Backspace", GLFW_KEY_BACKSPACE },
    { "Insert",    GLFW_KEY_INSERT },
    { "Delete",    GLFW_KEY_DELETE },
    { "Right",     GLFW_KEY_RIGHT },
    { "Left",      GLFW_KEY_LEFT },
    { "Down",      GLFW_KEY_DOWN },
    { "Up",        GLFW_KEY_UP },
    { "Page Up",   GLFW_KEY_PAGE_UP },
    { "Page Down", GLFW_KEY_PAGE_DOWN },
    { "Home",      GLFW_KEY_HOME },
    { "End",       GLFW_KEY_END },

    // Locks and special keys
    { "Caps Lock",    GLFW_KEY_CAPS_LOCK },
    { "Scroll Lock",  GLFW_KEY_SCROLL_LOCK },
    { "Num Lock",     GLFW_KEY_NUM_LOCK },
    { "Print Screen", GLFW_KEY_PRINT_SCREEN },
    { "Pause",        GLFW_KEY_PAUSE },

    // Function keys F1-F25
    { "F1",  GLFW_KEY_F1 },  { "F2",  GLFW_KEY_F2 },
    { "F3",  GLFW_KEY_F3 },  { "F4",  GLFW_KEY_F4 },
    { "F5",  GLFW_KEY_F5 },  { "F6",  GLFW_KEY_F6 },
    { "F7",  GLFW_KEY_F7 },  { "F8",  GLFW_KEY_F8 },
    { "F9",  GLFW_KEY_F9 },  { "F10", GLFW_KEY_F10 },
    { "F11", GLFW_KEY_F11 }, { "F12", GLFW_KEY_F12 },
    { "F13", GLFW_KEY_F13 }, { "F14", GLFW_KEY_F14 },
    { "F15", GLFW_KEY_F15 }, { "F16", GLFW_KEY_F16 },
    { "F17", GLFW_KEY_F17 }, { "F18", GLFW_KEY_F18 },
    { "F19", GLFW_KEY_F19 }, { "F20", GLFW_KEY_F20 },
    { "F21", GLFW_KEY_F21 }, { "F22", GLFW_KEY_F22 },
    { "F23", GLFW_KEY_F23 }, { "F24", GLFW_KEY_F24 },
    { "F25", GLFW_KEY_F25 },

    // Numeric keypad
    { "Keypad 0", GLFW_KEY_KP_0 },
    { "Keypad 1", GLFW_KEY_KP_1 },
    { "Keypad 2", GLFW_KEY_KP_2 },
    { "Keypad 3", GLFW_KEY_KP_3 },
    { "Keypad 4", GLFW_KEY_KP_4 },
    { "Keypad 5", GLFW_KEY_KP_5 },
    { "Keypad 6", GLFW_KEY_KP_6 },
    { "Keypad 7", GLFW_KEY_KP_7 },
    { "Keypad 8", GLFW_KEY_KP_8 },
    { "Keypad 9", GLFW_KEY_KP_9 },

    { "Keypad Decimal",  GLFW_KEY_KP_DECIMAL },
    { "Keypad Divide",   GLFW_KEY_KP_DIVIDE },
    { "Keypad Multiply", GLFW_KEY_KP_MULTIPLY },
    { "Keypad Subtract", GLFW_KEY_KP_SUBTRACT },
    { "Keypad Add",      GLFW_KEY_KP_ADD },
    { "Keypad Enter",    GLFW_KEY_KP_ENTER },
    { "Keypad Equal",    GLFW_KEY_KP_EQUAL },

    // Modifiers
    { "Left Shift",   GLFW_KEY_LEFT_SHIFT },
    { "Left Ctrl",    GLFW_KEY_LEFT_CONTROL },
    { "Left Alt",     GLFW_KEY_LEFT_ALT },
    { "Left Super",   GLFW_KEY_LEFT_SUPER },
    { "Right Shift",  GLFW_KEY_RIGHT_SHIFT },
    { "Right Ctrl",   GLFW_KEY_RIGHT_CONTROL },
    { "Right Alt",    GLFW_KEY_RIGHT_ALT },
    { "Right Super",  GLFW_KEY_RIGHT_SUPER },
    { "Menu",         GLFW_KEY_MENU },

    // Mouse buttons
    { "Mouse Left",   GLFW_MOUSE_BUTTON_LEFT },
    { "Mouse Right",  GLFW_MOUSE_BUTTON_RIGHT },
    { "Mouse Middle", GLFW_MOUSE_BUTTON_MIDDLE },
    { "Mouse 4",      GLFW_MOUSE_BUTTON_4 },
    { "Mouse 5",      GLFW_MOUSE_BUTTON_5 },
    { "Mouse 6",      GLFW_MOUSE_BUTTON_6 },
    { "Mouse 7",      GLFW_MOUSE_BUTTON_7 },
    { "Mouse 8",      GLFW_MOUSE_BUTTON_8 },

    { nullptr, 0 }
};


InputMapPanel::InputMapPanel(EditorContext* ctx) : m_ctx(ctx)
{
    m_inputMap = Services::Fetch<InputMap>();
}


const char* InputMapPanel::glfwKeyName(int key) const
{
    if (key == GLFW_KEY_UNKNOWN)
        return "Unbound";

    for (int i = 0; s_keys[i].name != nullptr; i++)
    {
        if (s_keys[i].code == key)
            return s_keys[i].name;
    }

    return "Unknown";
}


void InputMapPanel::Draw(bool& open)
{
    // Track key transitions so holding a key doesn't repeatedly capture it.
    static std::array<bool, GLFW_KEY_LAST + 1> previousDown{};

    const bool wasWaitingForKey = m_waitingForKey;
    int pressedInput = GLFW_KEY_UNKNOWN;

    GLFWwindow* window = glfwGetCurrentContext();

    if (window)
    {
        for (int i = 0; s_keys[i].name != nullptr; i++)
        {
            const int code = s_keys[i].code;

            if (code < 0 || code > GLFW_KEY_LAST)
                continue;

            const bool isMouse =
                code >= GLFW_MOUSE_BUTTON_1 &&
                code <= GLFW_MOUSE_BUTTON_LAST;

            const bool down = isMouse
                ? glfwGetMouseButton(window, code) == GLFW_PRESS
                : glfwGetKey(window, code) == GLFW_PRESS;

            if (down && !previousDown[code] &&
                pressedInput == GLFW_KEY_UNKNOWN)
            {
                pressedInput = code;
            }

            previousDown[code] = down;
        }
    }
    else
    {
        previousDown.fill(false);
    }

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(400, 200), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Input Map", &open))
    {
        ImGui::End();
        return;
    }

    drawToolbar();

    ImGui::Separator();

    drawBindingList();
    drawAddBindingPopup();
    drawEditBindingPopup();

    // Process captured input after drawing the popup.
    // This prevents clicking the capture/stop buttons from
    // accidentally being registered as mouse bindings.
    if (wasWaitingForKey && m_waitingForKey &&
        pressedInput != GLFW_KEY_UNKNOWN)
    {
        if (m_editingIndex >= 0)
            m_editKey = pressedInput;
        else
            m_newKey = pressedInput;

        m_waitingForKey = false;

        // Escape normally closes ImGui popups. Reopen the popup
        // after capturing it so it can be used as a binding.
        const char* popupName = m_editingIndex >= 0
            ? "EditBindingPopup"
            : "AddBindingPopup";

        if (!ImGui::IsPopupOpen("AddBindingPopup") &&
            !ImGui::IsPopupOpen("EditBindingPopup"))
        {
            ImGui::OpenPopup(popupName);
        }
    }
    else if (!ImGui::IsPopupOpen("AddBindingPopup") &&
        !ImGui::IsPopupOpen("EditBindingPopup"))
    {
        m_waitingForKey = false;
    }

    ImGui::End();
}

void InputMapPanel::Reload()
{
    m_bindings.clear();
    m_selectedIndex = -1;
    m_editingIndex = -1;
    m_waitingForKey = false;

    m_settings = Services::Fetch<ProjectSettings>();
    if (!m_settings)
        return;

    for (const auto& b : m_settings->Input())
    {
        BindingEntry e;
        e.action = b.action;
        e.glfwKey = b.key;
        m_bindings.push_back(std::move(e));
    }

    pushToInputMap();
}

void InputMapPanel::drawToolbar()
{
    if (ImGui::Button("+ Add Action"))
    {
        m_newAction[0] = '\0';
        m_newKey = GLFW_KEY_UNKNOWN;
        m_waitingForKey = false;

        ImGui::OpenPopup("AddBindingPopup");
    }

    ImGui::SameLine();

    if (m_selectedIndex == -1)
        ImGui::BeginDisabled();

    if (ImGui::Button("- Remove"))
    {
        if (m_selectedIndex >= 0 &&
            m_selectedIndex < (int)m_bindings.size())
        {
            m_bindings.erase(m_bindings.begin() + m_selectedIndex);
            m_selectedIndex = -1;

            applyBindings();
        }
    }

    if (m_selectedIndex == -1)
        ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Apply"))
        applyBindings();

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Apply bindings to InputMap service");
}


void InputMapPanel::drawBindingList()
{
    ImGui::PushStyleColor(
        ImGuiCol_ChildBg,
        ImVec4(0.15f, 0.15f, 0.15f, 1.0f)
    );

    ImGui::BeginChild("##inputmap_list", ImVec2(0, 0), false);

    float colW = ImGui::GetContentRegionAvail().x * 0.5f;

    ImGui::SetCursorPosX(8.0f);
    ImGui::TextDisabled("Action");

    ImGui::SameLine(colW);
    ImGui::TextDisabled("Key");

    ImGui::Separator();

    for (int i = 0; i < (int)m_bindings.size(); i++)
    {
        auto& b = m_bindings[i];

        bool selected = (m_selectedIndex == i);

        ImGui::PushID(i);

        if (ImGui::Selectable(
            "##row",
            selected,
            ImGuiSelectableFlags_SpanAllColumns,
            ImVec2(0, 22)))
        {
            m_selectedIndex = i;
        }

        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            m_editingIndex = i;
            m_editKey = b.glfwKey;
            m_waitingForKey = false;

            ImGui::OpenPopup("EditBindingPopup");
        }

        ImGui::SameLine(8.0f);
        ImGui::Text("%s", b.action.c_str());

        ImGui::SameLine(colW);

        ImGui::TextColored(
            ImVec4(0.44f, 0.70f, 1.0f, 1.0f),
            "%s",
            glfwKeyName(b.glfwKey)
        );

        ImGui::PopID();
    }

    if (m_bindings.empty())
    {
        ImGui::Spacing();

        float w = ImGui::GetContentRegionAvail().x;
        float tw = ImGui::CalcTextSize(
            "No actions defined. Click '+ Add Action' to start."
        ).x;

        ImGui::SetCursorPosX((w - tw) * 0.5f);

        ImGui::TextDisabled(
            "No actions defined. Click '+ Add Action' to start."
        );
    }

    ImGui::EndChild();

    ImGui::PopStyleColor();
}


void InputMapPanel::drawAddBindingPopup()
{
    if (!ImGui::BeginPopup("AddBindingPopup"))
        return;

    ImGui::Text("New Action");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetNextItemWidth(200.0f);

    ImGui::InputTextWithHint(
        "##newaction",
        "Action name (e.g. jump)",
        m_newAction,
        sizeof(m_newAction)
    );

    ImGui::Spacing();

    ImGui::Text("Key:");
    ImGui::SameLine();

    ImGui::TextColored(
        ImVec4(0.44f, 0.70f, 1.0f, 1.0f),
        "%s",
        glfwKeyName(m_newKey)
    );

    ImGui::Spacing();

    if (m_waitingForKey)
    {
        ImGui::TextColored(
            ImVec4(1.0f, 0.80f, 0.30f, 1.0f),
            "Listening... Press any key or mouse button."
        );

        if (ImGui::Button("Stop Listening"))
            m_waitingForKey = false;
    }
    else
    {
        if (ImGui::Button("Press a Key..."))
            m_waitingForKey = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool canAdd =
        m_newAction[0] != '\0' &&
        m_newKey != GLFW_KEY_UNKNOWN &&
        !m_waitingForKey;

    if (!canAdd)
        ImGui::BeginDisabled();

    if (ImGui::Button("Add", ImVec2(90, 0)))
    {
        BindingEntry entry;

        entry.action = m_newAction;
        entry.glfwKey = m_newKey;

        m_bindings.push_back(entry);

        applyBindings();

        m_waitingForKey = false;
        ImGui::CloseCurrentPopup();
    }

    if (!canAdd)
        ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(90, 0)))
    {
        m_waitingForKey = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}


void InputMapPanel::drawEditBindingPopup()
{
    if (m_editingIndex < 0 ||
        m_editingIndex >= (int)m_bindings.size())
    {
        return;
    }

    if (!ImGui::BeginPopup("EditBindingPopup"))
        return;

    auto& b = m_bindings[m_editingIndex];

    ImGui::Text("Edit: %s", b.action.c_str());

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Current Key:");
    ImGui::SameLine();

    ImGui::TextColored(
        ImVec4(0.44f, 0.70f, 1.0f, 1.0f),
        "%s",
        glfwKeyName(m_editKey)
    );

    ImGui::Spacing();

    if (m_waitingForKey)
    {
        ImGui::TextColored(
            ImVec4(1.0f, 0.80f, 0.30f, 1.0f),
            "Listening... Press any key or mouse button."
        );

        if (ImGui::Button("Stop Listening"))
            m_waitingForKey = false;
    }
    else
    {
        if (ImGui::Button("Press a Key..."))
            m_waitingForKey = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool canSave =
        m_editKey != GLFW_KEY_UNKNOWN &&
        !m_waitingForKey;

    if (!canSave)
        ImGui::BeginDisabled();

    if (ImGui::Button("Save", ImVec2(90, 0)))
    {
        b.glfwKey = m_editKey;

        applyBindings();

        m_editingIndex = -1;
        m_waitingForKey = false;

        ImGui::CloseCurrentPopup();
    }

    if (!canSave)
        ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(90, 0)))
    {
        m_editingIndex = -1;
        m_waitingForKey = false;

        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void InputMapPanel::pushToInputMap()
{
    if (!m_inputMap)
        return;

    m_inputMap->ClearBindings();
    for (const auto& b : m_bindings)
        m_inputMap->Bind(b.action, b.glfwKey);
}

void InputMapPanel::applyBindings()
{
    pushToInputMap();

    if (!m_settings)
        return;

    auto& out = m_settings->Input();
    out.clear();
    for (const auto& b : m_bindings)
        out.push_back({ b.action, b.glfwKey });

    if (!m_settings->Save())
        Debug::LogError("[Input] Failed to write settings.json");
}