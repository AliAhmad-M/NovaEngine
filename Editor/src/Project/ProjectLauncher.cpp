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

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "Shell32.lib")
#endif
#endif

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

    // Helper for checking whether an entry is a directory
    const auto isDirectory =
        [](const std::string& folder)
        {
            if (folder.empty())
                return false;

            std::error_code ec;
            return std::filesystem::is_directory(
                std::filesystem::path(folder), ec) && !ec;
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

    // Move a project directory to the Windows Recycle Bin
    const auto moveToRecycleBin = [](
        const std::string& folder,
        std::string& error) -> bool
        {
#ifdef _WIN32
            if (folder.empty())
            {
                error = "The project path is empty.";
                return false;
            }

            std::error_code ec;
            std::filesystem::path path =
                std::filesystem::absolute(folder, ec);

            if (ec)
            {
                error = "Could not resolve the project folder path.";
                return false;
            }

            path = path.lexically_normal();

            if (!std::filesystem::is_directory(path, ec) || ec)
            {
                error = "The project folder no longer exists or cannot be accessed.";
                return false;
            }

            // Do not allow a drive root or UNC share root to be recycled
            if (path == path.root_path())
            {
                error = "Refusing to delete a filesystem root.";
                return false;
            }

            std::wstring widePath = path.wstring();
            const std::wstring root = path.root_path().wstring();

            // SHFileOperation expects paths without a trailing separator
            while (widePath.size() > root.size() &&
                !widePath.empty() &&
                (widePath.back() == L'\\' || widePath.back() == L'/'))
            {
                widePath.pop_back();
            }

            widePath.push_back(L'\0');
            widePath.push_back(L'\0');

            SHFILEOPSTRUCTW operation{};
            operation.wFunc = FO_DELETE;
            operation.pFrom = widePath.c_str();
            operation.fFlags =
                FOF_ALLOWUNDO |
                FOF_NOCONFIRMATION |
                FOF_SILENT |
                FOF_NOERRORUI;

            const int result = SHFileOperationW(&operation);

            if (result != 0 || operation.fAnyOperationsAborted)
            {
                error = "Windows could not move the project to the Recycle Bin.";
                return false;
            }

            return true;
#else
            (void)folder;
            error = "Moving projects to the Recycle Bin is only supported on Windows.";
            return false;
#endif
        };

    std::string toOpen;
    std::string toForget;
    std::string toDelete;
    static std::string pendingDelete;

    // Header
    ImGui::Text("NOVA");
    ImGui::SameLine(0.0f, 12.0f);
    ImGui::TextDisabled("Project Launcher");

    ImGui::Spacing();
    ImGui::Separator();

    // Two pane layout
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
        "Double click to open. Use ... for project options.");

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
        const bool folderExists = isDirectory(e.path);

        ImGui::PushID(e.path.c_str());

        const float lineHeight =
            ImGui::GetTextLineHeightWithSpacing();

        const float rowHeight =
            lineHeight * 2.0f + 12.0f;

        const ImVec2 rowPos = ImGui::GetCursorScreenPos();

        const float rowWidth =
            ImGui::GetContentRegionAvail().x;
        const float menuButtonSize = ImGui::GetFrameHeight();
        const float menuButtonGap = ImGui::GetStyle().ItemSpacing.x;
        const float textWidth = std::max(
            1.0f,
            rowWidth - menuButtonSize - menuButtonGap);

        // Keep selectable separate from menu button
        const bool clicked = ImGui::Selectable(
            "##project",
            false,
            ImGuiSelectableFlags_AllowDoubleClick,
            ImVec2(textWidth, rowHeight));

        const bool doubleClicked =
            clicked && ImGui::IsMouseDoubleClicked(0);

        // Save the normal layout position
        const ImVec2 rowEnd = ImGui::GetCursorScreenPos();

        ImGui::SetCursorScreenPos(ImVec2(
            rowPos.x + rowWidth - menuButtonSize,
            rowPos.y + (rowHeight - menuButtonSize) * 0.5f));

        if (ImGui::Button("...", ImVec2(menuButtonSize, menuButtonSize)))
            ImGui::OpenPopup("##ProjectOptions");

        if (ImGui::BeginPopup("##ProjectOptions"))
        {
            if (ImGui::MenuItem("Remove from list"))
                toForget = e.path;

#ifdef _WIN32
            ImGui::BeginDisabled(!folderExists);
            if (ImGui::MenuItem("Delete project"))
                toDelete = e.path;
            ImGui::EndDisabled();
#else
            ImGui::BeginDisabled();
            ImGui::MenuItem("Delete project (Windows only)");
            ImGui::EndDisabled();
#endif
            ImGui::EndPopup();
        }

        // Clip project text
        ImGui::PushClipRect(
            rowPos,
            ImVec2(rowPos.x + textWidth, rowPos.y + rowHeight),
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
        std::string chosen = OpenNativeFolderDialog("Select Project Location", m_location);
        if (!chosen.empty())
        {
            copyToBuffer(
                m_location,
                sizeof(m_location),
                chosen);

            m_error.clear();
        }
    }

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
        std::string chosen = OpenNativeFolderDialog("Open Nova Project Folder", m_openPath[0] != '\0' ? m_openPath : m_location);
        if (!chosen.empty())
        {
            copyToBuffer(
                m_openPath,
                sizeof(m_openPath),
                chosen);

            m_error.clear();
        }
    }

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

    // Ask for confirmation before deleting
    if (!toDelete.empty())
    {
        pendingDelete = toDelete;
        ImGui::OpenPopup("Delete Project?");
    }

    if (ImGui::BeginPopupModal(
        "Delete Project?",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped(
            "Move this entire folder and all its contents to the Recycle Bin?");
        ImGui::Spacing();

        std::error_code displayPathError;
        const std::filesystem::path displayPath =
            std::filesystem::absolute(pendingDelete, displayPathError).lexically_normal();

        if (displayPathError)
        {
            ImGui::TextWrapped("%s", pendingDelete.c_str());
        }
        else
        {
            const std::string resolvedPath = displayPath.string();
            ImGui::TextWrapped("%s", resolvedPath.c_str());
        }

        ImGui::Spacing();

        if (ImGui::Button("Move to Recycle Bin", ImVec2(180.0f, 0.0f)))
        {
            std::string deleteError;
            if (moveToRecycleBin(pendingDelete, deleteError))
            {
                toForget = pendingDelete;
                m_error.clear();
            }
            else
            {
                m_error = deleteError;
            }

            pendingDelete.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
        {
            pendingDelete.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();   // Restore child WindowPadding
    ImGui::End();
    ImGui::PopStyleVar(2);  // Restore outer window styles

    // Apply changes after the recent-project list has finished iterating.
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