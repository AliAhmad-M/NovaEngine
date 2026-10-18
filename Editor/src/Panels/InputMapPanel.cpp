#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Panels/InputMapPanel.h>
#include <Core/Services.h>
#include <imgui.h>
#include <cstring>
#include <cstdio>

using namespace Nova::Editor;

const InputMapPanel::KeyEntry InputMapPanel::s_keys[] =
{
    { "A", GLFW_KEY_A }, { "B", GLFW_KEY_B }, { "C", GLFW_KEY_C },
    { "D", GLFW_KEY_D }, { "E", GLFW_KEY_E }, { "F", GLFW_KEY_F },
    { "G", GLFW_KEY_G }, { "H", GLFW_KEY_H }, { "I", GLFW_KEY_I },
    { "J", GLFW_KEY_J }, { "K", GLFW_KEY_K }, { "L", GLFW_KEY_L },
    { "M", GLFW_KEY_M }, { "N", GLFW_KEY_N }, { "O", GLFW_KEY_O },
    { "P", GLFW_KEY_P }, { "Q", GLFW_KEY_Q }, { "R", GLFW_KEY_R },
    { "S", GLFW_KEY_S }, { "T", GLFW_KEY_T }, { "U", GLFW_KEY_U },
    { "V", GLFW_KEY_V }, { "W", GLFW_KEY_W }, { "X", GLFW_KEY_X },
    { "Y", GLFW_KEY_Y }, { "Z", GLFW_KEY_Z },
    { "Space",        GLFW_KEY_SPACE        },
    { "Enter",        GLFW_KEY_ENTER        },
    { "Escape",       GLFW_KEY_ESCAPE       },
    { "Tab",          GLFW_KEY_TAB          },
    { "Backspace",    GLFW_KEY_BACKSPACE     },
    { "Delete",       GLFW_KEY_DELETE       },
    { "Up",           GLFW_KEY_UP           },
    { "Down",         GLFW_KEY_DOWN         },
    { "Left",         GLFW_KEY_LEFT         },
    { "Right",        GLFW_KEY_RIGHT        },
    { "Left Shift",   GLFW_KEY_LEFT_SHIFT   },
    { "Left Ctrl",    GLFW_KEY_LEFT_CONTROL },
    { "Left Alt",     GLFW_KEY_LEFT_ALT     },
    { "F1",  GLFW_KEY_F1  }, { "F2",  GLFW_KEY_F2  },
    { "F3",  GLFW_KEY_F3  }, { "F4",  GLFW_KEY_F4  },
    { "F5",  GLFW_KEY_F5  }, { "F6",  GLFW_KEY_F6  },
    { "F7",  GLFW_KEY_F7  }, { "F8",  GLFW_KEY_F8  },
    { "F9",  GLFW_KEY_F9  }, { "F10", GLFW_KEY_F10 },
    { "F11", GLFW_KEY_F11 }, { "F12", GLFW_KEY_F12 },
    { "0", GLFW_KEY_0 }, { "1", GLFW_KEY_1 }, { "2", GLFW_KEY_2 },
    { "3", GLFW_KEY_3 }, { "4", GLFW_KEY_4 }, { "5", GLFW_KEY_5 },
    { "6", GLFW_KEY_6 }, { "7", GLFW_KEY_7 }, { "8", GLFW_KEY_8 },
    { "9", GLFW_KEY_9 },
    { "Mouse Left",   GLFW_MOUSE_BUTTON_LEFT   },
    { "Mouse Right",  GLFW_MOUSE_BUTTON_RIGHT  },
    { "Mouse Middle", GLFW_MOUSE_BUTTON_MIDDLE },
    { nullptr, 0 }
};

InputMapPanel::InputMapPanel(EditorContext* ctx) : m_ctx(ctx)
{
    m_inputMap = Services::Fetch<InputMap>();
}

const char* InputMapPanel::glfwKeyName(int key) const
{
    for (int i = 0; s_keys[i].name != nullptr; i++)
        if (s_keys[i].code == key)
            return s_keys[i].name;
    return "Unknown";
}

void InputMapPanel::Draw(bool& open)
{
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(400, 200), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Input Map", &open))  // &open adds X close button
    {
        ImGui::End();
        return;
    }

    drawToolbar();
    ImGui::Separator();
    drawBindingList();
    drawAddBindingPopup();
    drawEditBindingPopup();

    ImGui::End();
}

void InputMapPanel::drawToolbar()
{
    if (ImGui::Button("+ Add Action"))
    {
        m_newAction[0] = '\0';
        ImGui::OpenPopup("AddBindingPopup");
    }

    ImGui::SameLine();

    if (m_selectedIndex == -1) ImGui::BeginDisabled();
    if (ImGui::Button("- Remove"))
    {
        if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_bindings.size())
        {
            m_bindings.erase(m_bindings.begin() + m_selectedIndex);
            m_selectedIndex = -1;
            applyBindings();
        }
    }
    if (m_selectedIndex == -1) ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Apply"))
        applyBindings();

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Apply bindings to InputMap service");
}

void InputMapPanel::drawBindingList()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
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

        if (ImGui::Selectable("##row", selected,
            ImGuiSelectableFlags_SpanAllColumns,
            ImVec2(0, 22)))
        {
            m_selectedIndex = i;
        }

        // Double click to edit
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            m_editingIndex = i;
            ImGui::OpenPopup("EditBindingPopup");
        }

        ImGui::SameLine(8.0f);
        ImGui::Text("%s", b.action.c_str());
        ImGui::SameLine(colW);
        ImGui::TextColored(
            ImVec4(0.44f, 0.70f, 1.0f, 1.0f),
            "%s", glfwKeyName(b.glfwKey)
        );

        ImGui::PopID();
    }

    if (m_bindings.empty())
    {
        ImGui::Spacing();
        float w = ImGui::GetContentRegionAvail().x;
        float tw = ImGui::CalcTextSize("No actions defined. Click '+ Add Action' to start.").x;
        ImGui::SetCursorPosX((w - tw) * 0.5f);
        ImGui::TextDisabled("No actions defined. Click '+ Add Action' to start.");
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void InputMapPanel::drawAddBindingPopup()
{
    if (ImGui::BeginPopup("AddBindingPopup"))
    {
        ImGui::Text("New Action");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(200.0f);
        ImGui::InputTextWithHint("##newaction", "Action name (e.g. jump)",
            m_newAction, sizeof(m_newAction));

        ImGui::Spacing();

        static int selectedKeyIndex = 0;
        ImGui::Text("Key:");
        ImGui::SetNextItemWidth(200.0f);

        if (ImGui::BeginCombo("##keyselect",
            s_keys[selectedKeyIndex].name ? s_keys[selectedKeyIndex].name : "Select..."))
        {
            for (int i = 0; s_keys[i].name != nullptr; i++)
            {
                bool sel = (selectedKeyIndex == i);
                if (ImGui::Selectable(s_keys[i].name, sel))
                    selectedKeyIndex = i;
                if (sel)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool canAdd = (m_newAction[0] != '\0' && s_keys[selectedKeyIndex].name != nullptr);

        if (!canAdd) ImGui::BeginDisabled();
        if (ImGui::Button("Add", ImVec2(90, 0)))
        {
            BindingEntry entry;
            entry.action = m_newAction;
            entry.glfwKey = s_keys[selectedKeyIndex].code;
            m_bindings.push_back(entry);
            applyBindings();
            ImGui::CloseCurrentPopup();
        }
        if (!canAdd) ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(90, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void InputMapPanel::drawEditBindingPopup()
{
    if (m_editingIndex < 0 || m_editingIndex >= (int)m_bindings.size()) return;

    if (ImGui::BeginPopup("EditBindingPopup"))
    {
        auto& b = m_bindings[m_editingIndex];

        ImGui::Text("Edit: %s", b.action.c_str());
        ImGui::Separator();
        ImGui::Spacing();

        static int selectedKeyIndex = 0;
        ImGui::Text("New Key:");
        ImGui::SetNextItemWidth(200.0f);

        if (ImGui::BeginCombo("##editkeyselect", s_keys[selectedKeyIndex].name))
        {
            for (int i = 0; s_keys[i].name != nullptr; i++)
            {
                bool sel = (selectedKeyIndex == i);
                if (ImGui::Selectable(s_keys[i].name, sel))
                    selectedKeyIndex = i;
                if (sel)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save", ImVec2(90, 0)))
        {
            b.glfwKey = s_keys[selectedKeyIndex].code;
            applyBindings();
            m_editingIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(90, 0)))
        {
            m_editingIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void InputMapPanel::applyBindings()
{
    if (!m_inputMap) return;

    for (const auto& b : m_bindings)
        m_inputMap->Bind(b.action, b.glfwKey);
}