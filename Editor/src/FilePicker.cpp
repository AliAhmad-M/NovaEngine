#include <Editor/FilePicker.h>
#include <imgui.h>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;
using namespace Nova::Editor;

bool Nova::Editor::DrawFilePicker(
    const char* id,
    std::string& currentPath,
    const char* filterExt,
    std::function<void(const std::string&)> onConfirm)
{
    bool open = true;
    ImGui::SetNextWindowSize(ImVec2(560, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::BeginPopupModal(id, &open, ImGuiWindowFlags_NoScrollbar))
        return false;

    struct State
    {
        fs::path   dir = fs::current_path();
        std::string selected = {};
        char        inputBuf[512] = {};
    };
    static State s;

    // Nav bar
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    ImGui::Text("%s", s.dir.string().c_str());
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
    if (ImGui::SmallButton("^ Up") && s.dir.has_parent_path())
    {
        s.dir = s.dir.parent_path();
        s.selected.clear();
    }
    ImGui::PopStyleVar();
    ImGui::Separator();

    // File list
    ImGui::BeginChild("##files", ImVec2(0, -80), true);

    std::vector<fs::directory_entry> dirs, files;
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(s.dir, ec))
    {
        if (entry.is_directory())        dirs.push_back(entry);
        else if (entry.is_regular_file())
        {
            if (filterExt && filterExt[0] != '\0'
                && entry.path().extension() != filterExt) continue;
            files.push_back(entry);
        }
    }
    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());

    for (auto& d : dirs)
    {
        std::string name = "[D] " + d.path().filename().string();
        if (ImGui::Selectable(name.c_str(), false))
        {
            s.dir = d.path();
            s.selected.clear();
            std::memset(s.inputBuf, 0, sizeof(s.inputBuf));
        }
    }
    for (auto& f : files)
    {
        std::string fname = f.path().filename().string();
        bool isSelected = (s.selected == f.path().string());
        if (ImGui::Selectable(fname.c_str(), isSelected))
        {
            s.selected = f.path().string();
            std::snprintf(s.inputBuf, sizeof(s.inputBuf), "%s", s.selected.c_str());
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            onConfirm(s.selected);
            ImGui::CloseCurrentPopup();
            ImGui::EndChild();
            ImGui::EndPopup();
            return false;
        }
    }

    ImGui::EndChild();

    // Bottom bar
    ImGui::Separator();
    ImGui::SetNextItemWidth(-120.0f);
    if (ImGui::InputText("##path", s.inputBuf, sizeof(s.inputBuf)))
        s.selected = s.inputBuf;

    ImGui::SameLine();
    if (ImGui::Button("Open", ImVec2(55, 0)) && !s.selected.empty())
    {
        onConfirm(s.selected);
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(55, 0)))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
    return open;
}