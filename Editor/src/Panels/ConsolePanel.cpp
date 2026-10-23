#include <Editor/Panels/ConsolePanel.h>
#include <Core/Debug.h>
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <cstring>   
#include <cstdio>    

using namespace Nova::Editor;

ConsolePanel::ConsolePanel(EditorContext* ctx) : m_ctx(ctx) {}

static bool ContainsNoCase(const std::string& hay, const char* needle)
{
    auto it = std::search(hay.begin(), hay.end(), needle, needle + strlen(needle),
        [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); });
    return it != hay.end();
}

void ConsolePanel::Draw()
{
    ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoMove);

    const auto& entries = Debug::GetEntries();

    // Count per level (shown on the toggle buttons, like Unity)
    int nLog = 0, nInfo = 0, nWarn = 0, nError = 0;
    for (const auto& e : entries)
    {
        switch (e.level)
        {
        case Nova::LogLevel::Info:  nInfo++;  break;
        case Nova::LogLevel::Warn:  nWarn++;  break;
        case Nova::LogLevel::Error: nError++; break;
        default:                    nLog++;   break;
        }
    }

    // Toolbar
    if (ImGui::Button("Clear")) { Debug::Clear(); m_selected = -1; }
    ImGui::SameLine();
    ImGui::Checkbox("Collapse", &m_collapse);
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::InputTextWithHint("##filter", "Filter...", m_filter, sizeof(m_filter));

    char label[32];
    ImGui::SameLine();
    std::snprintf(label, sizeof(label), "Log %d##log", nLog);
    ImGui::Checkbox(label, &m_showLog);
    ImGui::SameLine();
    std::snprintf(label, sizeof(label), "Info %d##info", nInfo);
    ImGui::Checkbox(label, &m_showInfo);
    ImGui::SameLine();
    std::snprintf(label, sizeof(label), "Warn %d##warn", nWarn);
    ImGui::Checkbox(label, &m_showWarn);
    ImGui::SameLine();
    std::snprintf(label, sizeof(label), "Error %d##error", nError);
    ImGui::Checkbox(label, &m_showError);

    ImGui::Separator();

    // Log list (leave room for the detail pane)
    const float detailH = 90.0f;
    ImGui::BeginChild("##console_scroll", ImVec2(0, -detailH), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;

    std::unordered_map<std::string, int> counts;
    if (m_collapse)
        for (const auto& e : entries)
            counts[std::to_string((int)e.level) + e.message]++;
    std::unordered_map<std::string, bool> seen;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    int row = 0;

    for (int i = 0; i < (int)entries.size(); i++)
    {
        const auto& entry = entries[i];

        if (entry.level == Nova::LogLevel::Log && !m_showLog)   continue;
        if (entry.level == Nova::LogLevel::Info && !m_showInfo)  continue;
        if (entry.level == Nova::LogLevel::Warn && !m_showWarn)  continue;
        if (entry.level == Nova::LogLevel::Error && !m_showError) continue;
        if (m_filter[0] != '\0' && !ContainsNoCase(entry.message, m_filter)) continue;

        int dupCount = 1;
        if (m_collapse)
        {
            std::string key = std::to_string((int)entry.level) + entry.message;
            if (seen[key]) continue;
            seen[key] = true;
            dupCount = counts[key];
        }

        ImVec4 color;
        const char* prefix;
        switch (entry.level)
        {
        case Nova::LogLevel::Info:  color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);   prefix = "[INFO] ";  break;
        case Nova::LogLevel::Warn:  color = ImVec4(1.0f, 0.85f, 0.0f, 1.0f);  prefix = "[WARN] ";  break;
        case Nova::LogLevel::Error: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);   prefix = "[ERROR] "; break;
        default:                    color = ImVec4(0.86f, 0.86f, 0.86f, 1.0f); prefix = "[LOG] ";   break;
        }

        // Striped background
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        if (row % 2 == 1)
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + ImGui::GetTextLineHeightWithSpacing()),
                IM_COL32(255, 255, 255, 10));
        row++;

        ImGui::PushID(i);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        bool clicked = ImGui::Selectable("##row", m_selected == i,
            ImGuiSelectableFlags_AllowOverlap, ImVec2(0, 0));
        ImGui::SameLine(0, 0);
        if (m_collapse && dupCount > 1)
            ImGui::Text("%s%s  (%d)", prefix, entry.message.c_str(), dupCount);
        else
            ImGui::Text("%s%s", prefix, entry.message.c_str());
        ImGui::PopStyleColor();

        if (clicked) m_selected = i;

        if (ImGui::BeginPopupContextItem("##rowctx"))
        {
            if (ImGui::MenuItem("Copy"))
                ImGui::SetClipboardText(entry.message.c_str());
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    // Only stick to the bottom if the user was already there and new logs arrived
    if (m_autoScroll && wasAtBottom && entries.size() != m_lastCount)
        ImGui::SetScrollHereY(1.0f);
    m_lastCount = entries.size();

    ImGui::EndChild();

    // Detail pane: full text of the selected message, selectable/copyable
    ImGui::BeginChild("##console_detail", ImVec2(0, 0), true);
    if (m_selected >= 0 && m_selected < (int)entries.size())
        ImGui::TextWrapped("%s", entries[m_selected].message.c_str());
    else
        ImGui::TextDisabled("Select a message to see details");
    ImGui::EndChild();

    ImGui::End();
}