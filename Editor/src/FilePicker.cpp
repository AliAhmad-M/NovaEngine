#include <Editor/FilePicker.h>
#include <imgui.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;
using namespace Nova::Editor;

bool Nova::Editor::DrawFilePicker(
    const char* id,
    std::string& currentPath,
    const char* filterExt,
    std::function<void(const std::string&)> onConfirm)
{
    ImGui::SetNextWindowSize(ImVec2(580, 420), ImGuiCond_FirstUseEver);

    bool open = true;
    if (!ImGui::BeginPopupModal(id, &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize))
        return false;

    struct State
    {
        fs::path    dir = fs::current_path();
        std::string selected;
        char        inputBuf[512] = {};
    };
    static State s;

    // Reset state
    if (ImGui::IsWindowAppearing())
    {
        s.selected.clear();
        std::memset(s.inputBuf, 0, sizeof(s.inputBuf));

        if (!currentPath.empty())
        {
            std::error_code ec;
            fs::path p(currentPath);

            // If it's a directory, navigate into it
            if (fs::is_directory(p, ec) && !ec)
            {
                s.dir = p;
            }
            // If it's a file, open its parent folder
            else if (fs::is_regular_file(p, ec) && !ec)
            {
                s.dir = p.parent_path();
                s.selected = p.string();
                std::snprintf(s.inputBuf, sizeof(s.inputBuf), "%s", s.selected.c_str());
            }

            else if (s.dir.empty() || !fs::exists(s.dir, ec))
            {
                fs::path parent = p.parent_path();
                if (fs::is_directory(parent, ec) && !ec)
                    s.dir = parent;
                else
                    s.dir = fs::current_path();
            }
        }
        else if (s.dir.empty())
        {
            s.dir = fs::current_path();
        }
    }

    // Nav bar
    const float navHeight = 28.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::BeginChild("##nav", ImVec2(0, navHeight), false, ImGuiWindowFlags_NoScrollbar);

    // Up button
    ImGui::SetCursorPosY((navHeight - ImGui::GetFrameHeight()) * 0.5f);
    ImGui::BeginDisabled(!s.dir.has_parent_path());
    if (ImGui::SmallButton("  ^  "))
    {
        s.dir = s.dir.parent_path();
        s.selected.clear();
        std::memset(s.inputBuf, 0, sizeof(s.inputBuf));
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0, 6);

    // Path display
    ImGui::SetCursorPosY((navHeight - ImGui::GetFrameHeight()) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextUnformatted(s.dir.string().c_str());
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // File list
    const float bottomBarH = 40.0f + ImGui::GetStyle().ItemSpacing.y * 2.0f;
    ImGui::BeginChild("##files",
        ImVec2(0, ImGui::GetContentRegionAvail().y - bottomBarH),
        true);

    // File list
    std::vector<fs::directory_entry> dirs, files;
    try
    {
        std::error_code ec;
        for (auto& entry : fs::directory_iterator(s.dir, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec) continue;
            std::error_code entryEc;
            if (entry.is_directory(entryEc) && !entryEc)
                dirs.push_back(entry);
            else if (entry.is_regular_file(entryEc) && !entryEc)
            {
                if (filterExt && filterExt[0] != '\0'
                    && entry.path().extension() != filterExt) continue;
                files.push_back(entry);
            }
        }
    }
    catch (const fs::filesystem_error&)
    {
        // Directory became inaccessible
        if (s.dir.has_parent_path())
            s.dir = s.dir.parent_path();
    }
    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());

    // Folder color
    const ImVec4 folderCol = ImVec4(0.85f, 0.65f, 0.20f, 1.00f);
    const ImVec4 fileCol = ImGui::GetStyleColorVec4(ImGuiCol_Text);

    for (auto& d : dirs)
    {
        std::string label = "  " + d.path().filename().string();
        bool isSelected = (s.selected == d.path().string());

        ImGui::PushStyleColor(ImGuiCol_Text, folderCol);
        if (ImGui::Selectable(("  [/] " + d.path().filename().string()).c_str(),
            isSelected,
            ImGuiSelectableFlags_AllowDoubleClick))
        {
            if (ImGui::IsMouseDoubleClicked(0))
            {
                // Navigate into directory
                s.dir = d.path();
                s.selected.clear();
                std::memset(s.inputBuf, 0, sizeof(s.inputBuf));
            }
            else
            {
                // Select the folder path
                s.selected = d.path().string();
                std::snprintf(s.inputBuf, sizeof(s.inputBuf), "%s", s.selected.c_str());
            }
        }
        ImGui::PopStyleColor();
    }

    for (auto& f : files)
    {
        bool isSelected = (s.selected == f.path().string());

        ImGui::PushStyleColor(ImGuiCol_Text, fileCol);
        if (ImGui::Selectable(("      " + f.path().filename().string()).c_str(),
            isSelected,
            ImGuiSelectableFlags_AllowDoubleClick))
        {
            s.selected = f.path().string();
            std::snprintf(s.inputBuf, sizeof(s.inputBuf), "%s", s.selected.c_str());

            if (ImGui::IsMouseDoubleClicked(0) && !s.selected.empty())
            {
                onConfirm(s.selected);
                ImGui::PopStyleColor();
                ImGui::EndChild();
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return false;
            }
        }
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    // Bottom bar
    ImGui::Separator();
    ImGui::Spacing();

    const float btnW = 64.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (btnW + spacing) * 2 - spacing);

    if (ImGui::InputText("##path", s.inputBuf, sizeof(s.inputBuf)))
        s.selected = s.inputBuf;

    ImGui::SameLine();
    ImGui::BeginDisabled(s.selected.empty());
    if (ImGui::Button("Open", ImVec2(btnW, 0)))
    {
        onConfirm(s.selected);
        ImGui::EndDisabled();
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return false;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(btnW, 0)))
    {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return false;
    }

    ImGui::EndPopup();
    return true;
}