#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Project/ProjectLauncher.h>
#include <Editor/FilePicker.h>

#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

using namespace Nova::Editor;

ProjectLauncher::ProjectLauncher()
{
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif

    std::string def =
        (std::filesystem::path(home ? home : ".") / "NovaProjects").string();

    std::strncpy(m_location, def.c_str(), sizeof(m_location) - 1);
    m_location[sizeof(m_location) - 1] = '\0';

    m_pickRoot = def;
}

void ProjectLauncher::Draw(
    ProjectManager& pm,
    const std::function<void(const std::string&)>& onOpen)
{
    ImGuiViewport* vp = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(16.0f, 12.0f));

    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(10.0f, 8.0f));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDocking;

    ImGui::Begin("##Launcher", nullptr, flags);

    // Helper for safely copying paths
    const auto copyToBuffer =
        [](char* dst, std::size_t capacity, const std::string& value)
        {
            if (capacity == 0)
                return;

            std::strncpy(dst, value.c_str(), capacity - 1);
            dst[capacity - 1] = '\0';
        };

    // Helper for validating a project
    const auto isProject =
        [](const std::string& folder)
        {
            if (folder.empty())
                return false;

            std::error_code ec;

            return std::filesystem::is_regular_file(
                std::filesystem::path(folder) / ProjectManager::kDescriptor,
                ec);
        };

	// Helper for normalizing a path
    const auto normalizePath = [](const std::string& value)
        {
            std::error_code ec;
            auto path = std::filesystem::weakly_canonical(value, ec);

            if (ec)
            {
                ec.clear();
                path = std::filesystem::path(value).lexically_normal();
            }

            std::string result = path.generic_string();

#ifdef _WIN32
            std::transform(
                result.begin(), result.end(), result.begin(),
                [](unsigned char c)
                {
                    return static_cast<char>(std::tolower(c));
                });
#endif

            return result;
        };

    std::string toOpen;
    std::string toForget;

    // Header
    ImGui::Text("NOVA");
    ImGui::SameLine(0.0f, 12.0f);
    ImGui::TextDisabled("Project Launcher");

    ImGui::Spacing();
    ImGui::Separator();

    // Two-pane layout
    ImGui::Dummy(ImVec2(1.0f, 8.0f));

    constexpr float outerInset = 12.0f;
    constexpr float paneGap = 12.0f;
    constexpr float bottomInset = 12.0f;

    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX() + outerInset);

    const ImVec2 available = ImGui::GetContentRegionAvail();

    const float paneWidth = available.x - outerInset;
    const float paneHeight = available.y - bottomInset;

    const float leftWidth = (paneWidth - paneGap) * 0.55f;
    const float rightWidth = paneWidth - leftWidth - paneGap;

    // Padding
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(12.0f, 10.0f));

    // Recent projects
    ImGui::BeginChild(
        "##recent",
        ImVec2(leftWidth, paneHeight),
        true,
        ImGuiWindowFlags_AlwaysUseWindowPadding);

    ImGui::Text("Recent Projects");
    ImGui::TextDisabled(
        "Double-click to open. Right-click to remove.");

    ImGui::Separator();
    ImGui::Spacing();

    if (pm.Recent().empty())
    {
        ImGui::Spacing();
        ImGui::TextDisabled("No recent projects.");
        ImGui::TextWrapped(
            "Create a new project or browse to an existing one "
            "to get started.");
    }

    for (const auto& e : pm.Recent())
    {
        const bool valid = isProject(e.path);

        ImGui::PushID(e.path.c_str());

        const float lineHeight =
            ImGui::GetTextLineHeightWithSpacing();

        const float rowHeight =
            lineHeight * 2.0f + 12.0f;

        const ImVec2 rowPos = ImGui::GetCursorScreenPos();

        const float rowWidth =
            ImGui::GetContentRegionAvail().x;

        const bool clicked = ImGui::Selectable(
            "##project",
            false,
            ImGuiSelectableFlags_AllowDoubleClick,
            ImVec2(0.0f, rowHeight));

        const bool doubleClicked =
            clicked && ImGui::IsMouseDoubleClicked(0);

        // Context menu belongs to the row
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Remove from list"))
                toForget = e.path;

            ImGui::EndPopup();
        }

        // Remember cursor position
        const ImVec2 rowEnd = ImGui::GetCursorScreenPos();

        // Clip the text to the row
        ImGui::PushClipRect(
            rowPos,
            ImVec2(rowPos.x + rowWidth, rowPos.y + rowHeight),
            true);

        ImGui::SetCursorScreenPos(
            ImVec2(rowPos.x + 9.0f, rowPos.y + 6.0f));

        if (!valid)
        {
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        }

        ImGui::TextUnformatted(e.name.c_str());

        if (!valid)
            ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos(
            ImVec2(
                rowPos.x + 9.0f,
                rowPos.y + 6.0f + lineHeight));

        ImGui::TextDisabled("%s", e.path.c_str());

        ImGui::PopClipRect();

        // Restore the layout
        ImGui::SetCursorScreenPos(rowEnd);

        if (doubleClicked && valid)
        {
            toOpen = e.path;
            m_error.clear();
        }

        ImGui::PopID();

        ImGui::Spacing();
    }

    ImGui::EndChild();
    ImGui::SameLine(0.0f, paneGap);

    // Create or open
    ImGui::BeginChild(
        "##actions",
        ImVec2(rightWidth, paneHeight),
        true,
        ImGuiWindowFlags_AlwaysUseWindowPadding);

    // Create project
    ImGui::Text("New Project");
    ImGui::TextDisabled(
        "Choose a name and the folder where it will be created.");

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Project Name");

    if (ImGui::InputTextWithHint(
        "##projectName",
        "My Game",
        m_name,
        sizeof(m_name)))
    {
        m_error.clear();
    }

    ImGui::Spacing();

    ImGui::Text("Parent Folder");

    constexpr float browseWidth = 76.0f;

    float fieldWidth =
        ImGui::GetContentRegionAvail().x
        - browseWidth
        - ImGui::GetStyle().ItemSpacing.x;

    if (fieldWidth < 1.0f)
        fieldWidth = 1.0f;

    ImGui::SetNextItemWidth(fieldWidth);

    if (ImGui::InputTextWithHint(
        "##location",
        "Select a parent folder",
        m_location,
        sizeof(m_location)))
    {
        m_error.clear();
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Browse##location",
        ImVec2(browseWidth, 0.0f)))
    {
        ImGui::OpenPopup("##LauncherPick");
    }

    // Set destination
    DrawFilePicker(
        "##LauncherPick",
        m_pickRoot,
        "",
        [this, copyToBuffer](const std::string& path)
        {
            copyToBuffer(
                m_location,
                sizeof(m_location),
                path);

            m_error.clear();
        });

    ImGui::Spacing();

    const bool canCreate =
        m_name[0] != '\0' &&
        m_location[0] != '\0';

    ImGui::BeginDisabled(!canCreate);

    if (ImGui::Button(
        "Create Project",
        ImVec2(-1.0f, ImGui::GetFrameHeight() * 1.2f)))
    {
        std::string root;
        std::string err;

        if (pm.Create(m_location, m_name, root, err))
        {
            toOpen = root;
            m_error.clear();
        }
        else
        {
            m_error = err;
        }
    }

    ImGui::EndDisabled();

    // Open existing project
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Text("Open Existing Project");
    ImGui::TextDisabled(
        "Choose a project folder containing its Nova descriptor.");

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetNextItemWidth(
        ImGui::GetContentRegionAvail().x
        - browseWidth
        - ImGui::GetStyle().ItemSpacing.x);

    if (ImGui::InputTextWithHint(
        "##openPath",
        "Path to project folder",
        m_openPath,
        sizeof(m_openPath)))
    {
        m_error.clear();
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Browse##open",
        ImVec2(browseWidth, 0.0f)))
    {
        ImGui::OpenPopup("##OpenProjectPick");
    }

    // Select project folder
    DrawFilePicker(
        "##OpenProjectPick",
        m_pickRoot,
        "",
        [this, copyToBuffer](const std::string& path)
        {
            copyToBuffer(
                m_openPath,
                sizeof(m_openPath),
                path);

            m_error.clear();
        });

    ImGui::Spacing();

    ImGui::BeginDisabled(m_openPath[0] == '\0');

    if (ImGui::Button(
        "Open Project",
        ImVec2(-1.0f, ImGui::GetFrameHeight() * 1.2f)))
    {
        const std::filesystem::path root(m_openPath);

        if (isProject(root.string()))
        {
            toOpen = root.lexically_normal().string();
            m_error.clear();
        }
        else
        {
            m_error =
                "The selected folder is not a valid Nova project. "
                "Expected project descriptor: " +
                std::string(ProjectManager::kDescriptor);
        }
    }

    ImGui::EndDisabled();

    // Errors
    if (!m_error.empty())
    {
        ImGui::Spacing();

        ImGui::PushStyleColor(
            ImGuiCol_Text,
            ImVec4(1.0f, 0.4f, 0.4f, 1.0f));

        ImGui::TextWrapped("%s", m_error.c_str());

        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();   // Restore child WindowPadding
    ImGui::End();
    ImGui::PopStyleVar(2);  // Restore outer window styles

    // Apply changes
    if (!toForget.empty())
        pm.Forget(toForget);

    if (!toOpen.empty())
    {
        const std::string target = normalizePath(toOpen);
        std::vector<std::string> existingEntries;

        for (const auto& entry : pm.Recent())
        {
            if (normalizePath(entry.path) == target)
                existingEntries.push_back(entry.path);
        }

        // Remove the existing matching entries
        for (const auto& path : existingEntries)
            pm.Forget(path);

        onOpen(toOpen);
    }
}