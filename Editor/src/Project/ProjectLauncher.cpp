#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Project/ProjectLauncher.h>
#include <Editor/FilePicker.h>
#include <imgui.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>

using namespace Nova::Editor;

ProjectLauncher::ProjectLauncher()
{
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    std::string def = (std::filesystem::path(home ? home : ".") / "NovaProjects").string();
    std::strncpy(m_location, def.c_str(), sizeof(m_location) - 1);
    m_pickRoot = def;
}

void ProjectLauncher::Draw(ProjectManager& pm, const std::function<void(const std::string&)>& onOpen)
{
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

    ImGui::Begin("##Launcher", nullptr, flags);

    float leftW = ImGui::GetContentRegionAvail().x * 0.55f;
    std::string toOpen, toForget;

    // Recent projects
    ImGui::BeginChild("##recent", ImVec2(leftW, 0), true);
    ImGui::TextDisabled("Recent Projects");
    ImGui::Separator();

    for (const auto& e : pm.Recent())
    {
        bool exists = std::filesystem::exists(std::filesystem::path(e.path) / ProjectManager::kDescriptor);
        if (!exists) ImGui::BeginDisabled();

        ImVec2 pos = ImGui::GetCursorPos();
        bool clicked = ImGui::Selectable(("##" + e.path).c_str(), false,
            ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 44));
        bool dbl = clicked && ImGui::IsMouseDoubleClicked(0);

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Remove from list")) toForget = e.path;
            ImGui::EndPopup();
        }

        ImVec2 end = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(pos.x + 8, pos.y + 4));
        ImGui::TextUnformatted(e.name.c_str());
        ImGui::SetCursorPosX(pos.x + 8);
        ImGui::TextDisabled("%s", e.path.c_str());
        ImGui::SetCursorPos(end);

        if (!exists) ImGui::EndDisabled();
        if (dbl && exists) toOpen = e.path;
    }
    if (pm.Recent().empty()) ImGui::TextDisabled("No projects yet");
    ImGui::EndChild();

    ImGui::SameLine();

    // New / open
    ImGui::BeginChild("##actions", ImVec2(0, 0), true);

    ImGui::TextDisabled("New Project");
    ImGui::Separator();
    ImGui::InputText("Name", m_name, sizeof(m_name));

    ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
    ImGui::InputText("##loc", m_location, sizeof(m_location));
    ImGui::SameLine();
    if (ImGui::Button("...##loc", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
        ImGui::OpenPopup("##LauncherPick");

    DrawFilePicker("##LauncherPick", m_pickRoot, "", [this](const std::string& path)
        {
            size_t slash = path.find_last_of("/\\");
            std::string dir = (slash != std::string::npos) ? path.substr(0, slash) : path;
            std::strncpy(m_location, dir.c_str(), sizeof(m_location) - 1);
        });

    if (ImGui::Button("Create Project", ImVec2(-1, 0)))
    {
        std::string root, err;
        if (pm.Create(m_location, m_name, root, err)) { toOpen = root; m_error.clear(); }
        else m_error = err;
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Open Existing");
    ImGui::Separator();
    ImGui::InputTextWithHint("##open", "Path to project folder", m_openPath, sizeof(m_openPath));
    if (ImGui::Button("Open", ImVec2(-1, 0)) && m_openPath[0] != '\0')
        toOpen = m_openPath;

    if (!m_error.empty())
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", m_error.c_str());
    }
    ImGui::EndChild();

    ImGui::End();

    if (!toForget.empty()) pm.Forget(toForget);
    if (!toOpen.empty())   onOpen(toOpen);
}