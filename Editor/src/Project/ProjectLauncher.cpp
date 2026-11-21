#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Project/ProjectLauncher.h>
#include <Editor/FilePicker.h>
#include <Editor/Theme.h>

#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

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

// Helpers
namespace
{
    using Theme::U32;

    ImFont* BoldFont()
    {
        ImGuiIO& io = ImGui::GetIO();
        return io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : ImGui::GetFont();
    }

    std::string Lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    // "My Cool Game" -> "MC", "Platformer" -> "PL"
    std::string Initials(const std::string& name)
    {
        std::string letters, firsts;
        bool start = true;

        for (unsigned char c : name)
        {
            if (!std::isalnum(c))
            {
                start = true;
                continue;
            }

            const char up = static_cast<char>(std::toupper(c));
            letters.push_back(up);

            if (start)
            {
                firsts.push_back(up);
                start = false;
            }
        }

        std::string out = firsts.size() >= 2
            ? firsts.substr(0, 2)
            : letters.substr(0, std::min<std::size_t>(2, letters.size()));

        return out.empty() ? std::string("N") : out;
    }

    std::string RelativeTime(std::int64_t t)
    {
        if (t <= 0)
            return "Never opened";

        // Tolerate millisecond timestamps
        if (t > 100000000000LL)
            t /= 1000;

        const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
        std::int64_t d = now - t;
        if (d < 0)
            d = 0;

        char buf[64];

        if (d < 60)
            return "Just now";

        if (d < 3600)
        {
            std::snprintf(buf, sizeof(buf), "%lld min ago", static_cast<long long>(d / 60));
            return buf;
        }

        if (d < 86400)
        {
            std::snprintf(buf, sizeof(buf), "%lld hr ago", static_cast<long long>(d / 3600));
            return buf;
        }

        if (d < 86400LL * 30)
        {
            const long long days = static_cast<long long>(d / 86400);
            std::snprintf(buf, sizeof(buf), days == 1 ? "%lld day ago" : "%lld days ago", days);
            return buf;
        }

        std::tm tmv{};
        const std::time_t tt = static_cast<std::time_t>(t);
#ifdef _WIN32
        localtime_s(&tmv, &tt);
#else
        localtime_r(&tt, &tmv);
#endif
        std::strftime(buf, sizeof(buf), "%b %d, %Y", &tmv);
        return buf;
    }

    void CopyToBuffer(char* dst, std::size_t capacity, const std::string& value)
    {
        if (capacity == 0)
            return;

        std::strncpy(dst, value.c_str(), capacity - 1);
        dst[capacity - 1] = '\0';
    }

    bool IsProjectFolder(const std::string& folder)
    {
        if (folder.empty())
            return false;

        std::error_code ec;
        return std::filesystem::is_regular_file(
            std::filesystem::path(folder) / ProjectManager::kDescriptor, ec);
    }

    bool IsDirectory(const std::string& folder)
    {
        if (folder.empty())
            return false;

        std::error_code ec;
        return std::filesystem::is_directory(std::filesystem::path(folder), ec) && !ec;
    }

    std::string NormalizePath(const std::string& value)
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
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
#endif

        return result;
    }

    bool IsValidProjectName(const char* name)
    {
        if (!name || !*name)
            return false;

        for (const char* p = name; *p; ++p)
        {
            if (std::strchr("\\/:*?\"<>|", *p))
                return false;
        }

        const char last = name[std::strlen(name) - 1];
        return last != ' ' && last != '.';
    }

    void RevealInExplorer(const std::string& folder)
    {
#ifdef _WIN32
        std::error_code ec;
        const auto path = std::filesystem::absolute(folder, ec);
        if (ec)
            return;

        ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
        (void)folder;
#endif
    }

    // Move a project directory to the Windows Recycle Bin
    bool MoveToRecycleBin(const std::string& folder, std::string& error)
    {
#ifdef _WIN32
        if (folder.empty())
        {
            error = "The project path is empty.";
            return false;
        }

        std::error_code ec;
        std::filesystem::path path = std::filesystem::absolute(folder, ec);

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
    }

    // Flat placeholder
    void DrawPlaceholder(
        ImDrawList* dl,
        const ImVec2& min,
        const ImVec2& max,
        const std::string& name,
        bool valid,
        float rounding)
    {
        dl->AddRectFilled(min, max, U32(Theme::Sunken()), rounding, ImDrawFlags_RoundCornersTop);

        const std::string ini = Initials(name);
        ImFont* font = BoldFont();
        const float fs = font->FontSize * 1.5f;
        const ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, ini.c_str());
        const ImVec2 c((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);

        dl->AddText(
            font, fs,
            ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f),
            U32(Theme::TextDim(), valid ? 0.75f : 0.35f),
            ini.c_str());
    }

    void DrawPinIcon(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 col)
    {
        const ImVec2 c((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);

        dl->AddRectFilled(ImVec2(c.x - 3.5f, c.y - 8.0f), ImVec2(c.x + 3.5f, c.y - 2.0f), col, 1.5f);
        dl->AddLine(ImVec2(c.x - 6.0f, c.y - 1.5f), ImVec2(c.x + 6.0f, c.y - 1.5f), col, 2.0f);
        dl->AddLine(ImVec2(c.x, c.y - 1.0f), ImVec2(c.x, c.y + 7.0f), col, 1.6f);
    }

    void DrawDotsIcon(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 col)
    {
        const ImVec2 c((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);

        dl->AddCircleFilled(ImVec2(c.x - 5.0f, c.y), 1.9f, col);
        dl->AddCircleFilled(ImVec2(c.x, c.y), 1.9f, col);
        dl->AddCircleFilled(ImVec2(c.x + 5.0f, c.y), 1.9f, col);
    }
}

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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32.0f, 24.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("##Launcher", nullptr, flags);
    ImGui::PopStyleVar(3);

    ImDrawList* wdl = ImGui::GetWindowDrawList();
    ImFont* bold = BoldFont();

    // Deferred actions
    std::string toOpen;
    std::string toForget;
    std::string toDelete;
    std::string pinPath;
    bool pinValue = false;
    bool pinRequested = false;

    // Header
    {
        ImGui::PushFont(bold);
        ImGui::TextUnformatted("Nova");
        ImGui::PopFont();

        ImGui::SameLine(0.0f, 10.0f);
        ImGui::TextDisabled("Project Launcher");

        ImGui::SameLine();
        const float vw = ImGui::CalcTextSize("v0.1.0").x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - vw);
        ImGui::TextDisabled("v0.1.0");
    }

    ImGui::Dummy(ImVec2(1.0f, 18.0f));

    // Toolbar
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));

    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(38.0f, 8.0f));
        ImGui::SetNextItemWidth(std::min(380.0f, ImGui::GetContentRegionAvail().x * 0.38f));
        ImGui::InputTextWithHint("##search", "Search projects", m_search, sizeof(m_search));
        ImGui::PopStyleVar();

        const ImVec2 imin = ImGui::GetItemRectMin();
        const ImVec2 imax = ImGui::GetItemRectMax();
        const ImVec2 c(imin.x + 20.0f, (imin.y + imax.y) * 0.5f - 1.0f);
        const ImU32 ic = U32(Theme::TextDim());

        wdl->AddCircle(c, 5.5f, ic, 0, 1.6f);
        wdl->AddLine(ImVec2(c.x + 4.0f, c.y + 4.0f), ImVec2(c.x + 9.0f, c.y + 9.0f), ic, 1.6f);
    }

    ImGui::SameLine(0.0f, 18.0f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Sort");
    ImGui::SameLine();

    {
        static const char* sortNames[] = { "Last opened", "Name (A-Z)", "Name (Z-A)" };

        ImGui::SetNextItemWidth(150.0f);
        if (ImGui::BeginCombo("##sort", sortNames[m_sort]))
        {
            for (int i = 0; i < 3; ++i)
            {
                if (ImGui::Selectable(sortNames[i], m_sort == i))
                    m_sort = i;
            }
            ImGui::EndCombo();
        }
    }

    ImGui::SameLine(0.0f, 18.0f);
    ImGui::Checkbox("Pinned only", &m_pinnedOnly);

    {
        constexpr float openW = 150.0f;
        constexpr float newW = 150.0f;

        ImGui::SameLine();
        ImGui::SetCursorPosX(
            ImGui::GetCursorPosX() +
            std::max(0.0f, ImGui::GetContentRegionAvail().x - openW - newW - ImGui::GetStyle().ItemSpacing.x));

        if (ImGui::Button("Open Folder...", ImVec2(openW, 0.0f)))
        {
            const std::string chosen =
                OpenNativeFolderDialog("Open Nova Project Folder", m_location);

            if (!chosen.empty())
            {
                if (IsProjectFolder(chosen))
                {
                    toOpen = std::filesystem::path(chosen).lexically_normal().string();
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
        }

        ImGui::SameLine();

        // Accent
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::Accent());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::AccentHover());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::AccentActive());
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

        if (ImGui::Button("New Project", ImVec2(newW, 0.0f)))
            m_openNewModal = true;

        ImGui::PopStyleColor(4);
    }

    ImGui::PopStyleVar();

    // Error banner
    if (!m_error.empty() && !ImGui::IsPopupOpen("New Project##modal"))
    {
        ImGui::Dummy(ImVec2(1.0f, 4.0f));

        const ImVec2 bp = ImGui::GetCursorScreenPos();
        const float bw = ImGui::GetContentRegionAvail().x;
        const float textW = bw - 96.0f;
        const float textH = ImGui::CalcTextSize(m_error.c_str(), nullptr, false, textW).y;
        const float boxH = std::max(42.0f, textH + 22.0f);

        wdl->AddRectFilled(bp, ImVec2(bp.x + bw, bp.y + boxH), U32(Theme::Danger(), 0.12f), 4.0f);
        wdl->AddRect(bp, ImVec2(bp.x + bw, bp.y + boxH), U32(Theme::Danger(), 0.5f), 4.0f);

        ImGui::SetCursorScreenPos(ImVec2(bp.x + 16.0f, bp.y + 11.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::Danger());
        ImGui::PushTextWrapPos(bp.x + 16.0f + textW);
        ImGui::TextUnformatted(m_error.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos(ImVec2(bp.x + bw - 76.0f, bp.y + (boxH - ImGui::GetFrameHeight()) * 0.5f));
        if (ImGui::SmallButton("Dismiss"))
            m_error.clear();

        ImGui::SetCursorScreenPos(bp);
        ImGui::Dummy(ImVec2(bw, boxH));
    }

    ImGui::Dummy(ImVec2(1.0f, 12.0f));

    std::vector<const ProjectEntry*> pinnedItems;
    std::vector<const ProjectEntry*> otherItems;

    // Build the visible, sorted project list
    {
        const std::string needle = Lower(m_search);
        std::vector<const ProjectEntry*> items;

        for (const auto& e : pm.Recent())
        {
            if (m_pinnedOnly && !e.pinned)
                continue;

            if (!needle.empty() &&
                Lower(e.name).find(needle) == std::string::npos &&
                Lower(e.path).find(needle) == std::string::npos)
                continue;

            items.push_back(&e);
        }

        const int sortMode = m_sort;
        std::stable_sort(items.begin(), items.end(),
            [sortMode](const ProjectEntry* a, const ProjectEntry* b)
            {
                switch (sortMode)
                {
                case 1:  return Lower(a->name) < Lower(b->name);
                case 2:  return Lower(a->name) > Lower(b->name);
                default: return a->lastOpened > b->lastOpened;
                }
            });

        for (const ProjectEntry* e : items)
            (e->pinned ? pinnedItems : otherItems).push_back(e);
    }

    constexpr float cardRounding = 6.0f;

    // Card drawing
    auto drawNewTile = [&](const ImVec2& size, float thumbH)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();

            const ImVec2 p0 = ImGui::GetCursorScreenPos();
            const ImVec2 p1(p0.x + size.x, p0.y + size.y);
            const bool over = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(p0, p1);

            if (over)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                if (ImGui::IsMouseClicked(0))
                    m_openNewModal = true;
            }

            if (over)
                dl->AddRectFilled(p0, p1, U32(Theme::Raised(), 0.5f), cardRounding);

            dl->AddRect(p0, p1,
                over ? U32(Theme::Accent()) : U32(Theme::BorderStrong()),
                cardRounding, 0, 1.0f);

            const ImVec2 c((p0.x + p1.x) * 0.5f, p0.y + thumbH * 0.5f);
            const ImU32 mark = over ? U32(Theme::Accent()) : U32(Theme::TextDim());

            dl->AddLine(ImVec2(c.x - 9.0f, c.y), ImVec2(c.x + 9.0f, c.y), mark, 1.6f);
            dl->AddLine(ImVec2(c.x, c.y - 9.0f), ImVec2(c.x, c.y + 9.0f), mark, 1.6f);

            const float infoY = p0.y + thumbH;
            const float smallSize = ImGui::GetFontSize() * 0.88f;

            dl->AddText(bold, bold->FontSize,
                ImVec2(p0.x + 12.0f, infoY + 10.0f),
                U32(Theme::Text()), "New project");

            dl->AddText(ImGui::GetFont(), smallSize,
                ImVec2(p0.x + 12.0f, infoY + 10.0f + bold->FontSize + 4.0f),
                U32(Theme::TextDim()), "Empty project");

            ImGui::Dummy(size);
        };

    auto drawCard = [&](const ProjectEntry& e, const ImVec2& size, float thumbH)
        {
            const bool valid = IsProjectFolder(e.path);
            const bool folderExists = IsDirectory(e.path);

            ImGui::PushID(e.path.c_str());

            ImDrawList* dl = ImGui::GetWindowDrawList();

            constexpr float hit = 28.0f;

            const ImVec2 p0 = ImGui::GetCursorScreenPos();
            const ImVec2 p1(p0.x + size.x, p0.y + size.y);
            const ImVec2 thumbMax(p1.x, p0.y + thumbH);

            const ImVec2 pinMin(p0.x + 6.0f, p0.y + 6.0f);
            const ImVec2 pinMax(pinMin.x + hit, pinMin.y + hit);
            const ImVec2 menuMin(p1.x - 6.0f - hit, p0.y + 6.0f);
            const ImVec2 menuMax(menuMin.x + hit, menuMin.y + hit);

            const bool overCard = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(p0, p1);
            const bool overPin = overCard && ImGui::IsMouseHoveringRect(pinMin, pinMax);
            const bool overMenu = overCard && ImGui::IsMouseHoveringRect(menuMin, menuMax);
            const bool selected = (m_selected == e.path);

            // Interaction
            if (overCard)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

                if (ImGui::IsMouseClicked(0))
                {
                    if (overPin)
                    {
                        pinRequested = true;
                        pinPath = e.path;
                        pinValue = !e.pinned;
                    }
                    else if (overMenu)
                    {
                        m_selected = e.path;
                        ImGui::OpenPopup("##ctx");
                    }
                    else
                    {
                        m_selected = e.path;
                    }
                }

                if (!overPin && !overMenu && ImGui::IsMouseDoubleClicked(0))
                {
                    if (valid)
                    {
                        toOpen = e.path;
                        m_error.clear();
                    }
                    else
                    {
                        m_error =
                            "Cannot open \"" + e.name + "\": " +
                            std::string(ProjectManager::kDescriptor) +
                            " was not found in " + e.path;
                    }
                }

                if (ImGui::IsMouseClicked(1))
                {
                    m_selected = e.path;
                    ImGui::OpenPopup("##ctx");
                }
            }

            // Card body
            dl->AddRectFilled(
                p0, p1,
                U32(overCard ? Theme::RaisedHover() : Theme::Raised()),
                cardRounding);

            DrawPlaceholder(dl, p0, thumbMax, e.name, valid, cardRounding);

            dl->AddRect(
                p0, p1,
                selected ? U32(Theme::Accent())
                : overCard ? U32(Theme::BorderStrong())
                : U32(Theme::Border()),
                cardRounding, 0, selected ? 2.0f : 1.0f);

            // Pin
            if (e.pinned)
                DrawPinIcon(dl, pinMin, pinMax, U32(Theme::Accent()));
            else if (overCard)
                DrawPinIcon(dl, pinMin, pinMax, overPin ? U32(Theme::Text()) : U32(Theme::TextDim()));

            if (overCard)
                DrawDotsIcon(dl, menuMin, menuMax, overMenu ? U32(Theme::Text()) : U32(Theme::TextDim()));

            // Info text
            {
                const float infoY = thumbMax.y;
                const ImVec4 clip(p0.x + 12.0f, infoY, p1.x - 12.0f, p1.y);
                const float smallSize = ImGui::GetFontSize() * 0.88f;
                ImFont* regular = ImGui::GetFont();

                dl->AddText(bold, bold->FontSize,
                    ImVec2(p0.x + 12.0f, infoY + 10.0f),
                    valid ? U32(Theme::Text()) : U32(Theme::TextDim()),
                    e.name.c_str(), nullptr, 0.0f, &clip);

                dl->AddText(regular, smallSize,
                    ImVec2(p0.x + 12.0f, infoY + 10.0f + bold->FontSize + 4.0f),
                    U32(Theme::TextDim()),
                    e.path.c_str(), nullptr, 0.0f, &clip);

                // Last-opened time or error
                const std::string status = valid
                    ? RelativeTime(e.lastOpened)
                    : (folderExists ? "Not a Nova project" : "Folder missing");

                dl->AddText(regular, smallSize,
                    ImVec2(p0.x + 12.0f, p1.y - 12.0f - smallSize),
                    valid ? U32(Theme::TextDim(), 0.85f) : U32(Theme::Danger()),
                    status.c_str(), nullptr, 0.0f, &clip);
            }

            // Reserve the layout space
            ImGui::Dummy(size);

            // Context menu
            if (ImGui::BeginPopup("##ctx"))
            {
                ImGui::BeginDisabled(!valid);
                if (ImGui::MenuItem("Open"))
                {
                    toOpen = e.path;
                    m_error.clear();
                }
                ImGui::EndDisabled();

                if (ImGui::MenuItem(e.pinned ? "Unpin" : "Pin to top"))
                {
                    pinRequested = true;
                    pinPath = e.path;
                    pinValue = !e.pinned;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Copy path"))
                    ImGui::SetClipboardText(e.path.c_str());

#ifdef _WIN32
                ImGui::BeginDisabled(!folderExists);
                if (ImGui::MenuItem("Show in Explorer"))
                    RevealInExplorer(e.path);
                ImGui::EndDisabled();
#endif

                ImGui::Separator();

                if (ImGui::MenuItem("Remove from list"))
                    toForget = e.path;

#ifdef _WIN32
                ImGui::BeginDisabled(!folderExists);
                ImGui::PushStyleColor(ImGuiCol_Text, Theme::Danger());
                if (ImGui::MenuItem("Delete project..."))
                    toDelete = e.path;
                ImGui::PopStyleColor();
                ImGui::EndDisabled();
#else
                ImGui::BeginDisabled();
                ImGui::MenuItem("Delete project (Windows only)");
                ImGui::EndDisabled();
#endif
                ImGui::EndPopup();
            }

            ImGui::PopID();
        };

    auto drawGrid = [&](const std::vector<const ProjectEntry*>& list, bool withNewTile)
        {
            constexpr float gap = 16.0f;
            constexpr float minCardW = 232.0f;
            constexpr float infoH = 84.0f;

            const float availW = ImGui::GetContentRegionAvail().x;
            const int cols = std::max(1, static_cast<int>((availW + gap) / (minCardW + gap)));
            const float cardW = std::floor((availW - gap * static_cast<float>(cols - 1)) / static_cast<float>(cols));
            const float thumbH = std::clamp(std::floor(cardW * 0.56f), 110.0f, 170.0f);
            const ImVec2 cardSize(cardW, thumbH + infoH);

            int index = 0;
            auto place = [&]()
                {
                    if (index % cols != 0)
                        ImGui::SameLine();
                    ++index;
                };

            if (withNewTile)
            {
                place();
                drawNewTile(cardSize, thumbH);
            }

            for (const ProjectEntry* e : list)
            {
                place();
                drawCard(*e, cardSize, thumbH);
            }
        };

    auto heading = [&](const char* text)
        {
            ImGui::PushFont(bold);
            ImGui::TextUnformatted(text);
            ImGui::PopFont();
        };

    const float gridH = std::max(60.0f, ImGui::GetContentRegionAvail().y);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(16.0f, 16.0f));

    ImGui::BeginChild("##grid", ImVec2(0.0f, gridH), false);

    if (!pinnedItems.empty())
    {
        heading("Pinned");
        drawGrid(pinnedItems, false);
        ImGui::Dummy(ImVec2(1.0f, 10.0f));
        heading("Recent");
    }

    drawGrid(otherItems, !m_pinnedOnly);

    if (pm.Recent().empty())
    {
        ImGui::Dummy(ImVec2(1.0f, 6.0f));
        ImGui::TextDisabled("No projects yet. Create one, or use Open Folder to add an existing project.");
    }
    else if (pinnedItems.empty() && otherItems.empty())
    {
        ImGui::Dummy(ImVec2(1.0f, 6.0f));
        ImGui::TextDisabled("No projects match your search or filter.");
    }

    ImGui::Dummy(ImVec2(1.0f, 8.0f));
    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    // New Project dialog
    if (m_openNewModal)
    {
        m_name[0] = '\0';
        m_error.clear();
        ImGui::OpenPopup("New Project##modal");
        m_openNewModal = false;
    }

    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(540.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 22.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);

    const bool newOpen = ImGui::BeginPopupModal(
        "New Project##modal", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::PopStyleVar(2);

    if (newOpen)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            ImGui::CloseCurrentPopup();

        ImGui::PushFont(bold);
        ImGui::TextUnformatted("New Project");
        ImGui::PopFont();
        ImGui::TextDisabled("Pick a name and where the project folder will be created.");

        ImGui::Dummy(ImVec2(1.0f, 8.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));

        const bool nameValid = IsValidProjectName(m_name);
        const bool canCreate = nameValid && m_location[0] != '\0';

        auto tryCreate = [&]()
            {
                std::string root;
                std::string err;

                if (pm.Create(m_location, m_name, root, err))
                {
                    toOpen = root;
                    m_error.clear();
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    m_error = err;
                }
            };

        ImGui::Text("Project name");

        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();

        ImGui::SetNextItemWidth(-FLT_MIN);
        const bool enter = ImGui::InputTextWithHint(
            "##projectName", "My Game", m_name, sizeof(m_name),
            ImGuiInputTextFlags_EnterReturnsTrue);

        if (ImGui::IsItemEdited())
            m_error.clear();

        if (m_name[0] != '\0' && !nameValid)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::Danger());
            ImGui::TextUnformatted("Names can't contain \\ / : * ? \" < > |  or end with a space or dot.");
            ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(1.0f, 4.0f));
        ImGui::Text("Location");

        constexpr float browseW = 88.0f;
        ImGui::SetNextItemWidth(
            std::max(1.0f, ImGui::GetContentRegionAvail().x - browseW - ImGui::GetStyle().ItemSpacing.x));

        if (ImGui::InputTextWithHint(
            "##location", "Select a parent folder", m_location, sizeof(m_location)))
            m_error.clear();

        ImGui::SameLine();

        if (ImGui::Button("Browse##location", ImVec2(browseW, 0.0f)))
        {
            const std::string chosen =
                OpenNativeFolderDialog("Select Project Location", m_location);

            if (!chosen.empty())
            {
                CopyToBuffer(m_location, sizeof(m_location), chosen);
                m_error.clear();
            }
        }

        if (nameValid && m_location[0] != '\0')
        {
            const std::string full =
                (std::filesystem::path(m_location) / m_name).string();

            ImGui::Dummy(ImVec2(1.0f, 2.0f));
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextDisabled("Will be created at  %s", full.c_str());
            ImGui::PopTextWrapPos();
        }

        if (!m_error.empty())
        {
            ImGui::Dummy(ImVec2(1.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::Danger());
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(m_error.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(1.0f, 12.0f));

        constexpr float cancelW = 100.0f;
        constexpr float createW = 150.0f;

        ImGui::SetCursorPosX(
            ImGui::GetCursorPosX() +
            std::max(0.0f, ImGui::GetContentRegionAvail().x - cancelW - createW - ImGui::GetStyle().ItemSpacing.x));

        if (ImGui::Button("Cancel", ImVec2(cancelW, 0.0f)))
            ImGui::CloseCurrentPopup();

        ImGui::SameLine();

        ImGui::BeginDisabled(!canCreate);
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::Accent());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::AccentHover());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::AccentActive());
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

        const bool createClicked = ImGui::Button("Create Project", ImVec2(createW, 0.0f));

        ImGui::PopStyleColor(4);
        ImGui::EndDisabled();

        if ((createClicked || enter) && canCreate)
            tryCreate();

        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }

    // Delete confirmation
    if (!toDelete.empty())
    {
        m_pendingDelete = toDelete;
        ImGui::OpenPopup("Delete Project?");
    }

    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 22.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);

    const bool deleteOpen = ImGui::BeginPopupModal(
        "Delete Project?", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::PopStyleVar(2);

    if (deleteOpen)
    {
        ImGui::TextWrapped("Move this entire folder and all its contents to the Recycle Bin?");
        ImGui::Spacing();

        std::error_code displayPathError;
        const std::filesystem::path displayPath =
            std::filesystem::absolute(m_pendingDelete, displayPathError).lexically_normal();

        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextDim());
        if (displayPathError)
            ImGui::TextWrapped("%s", m_pendingDelete.c_str());
        else
            ImGui::TextWrapped("%s", displayPath.string().c_str());
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(1.0f, 10.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));

        ImGui::PushStyleColor(ImGuiCol_Button, Theme::Danger());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.96f, 0.44f, 0.44f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.80f, 0.32f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

        if (ImGui::Button("Move to Recycle Bin", ImVec2(200.0f, 0.0f)))
        {
            std::string deleteError;
            if (MoveToRecycleBin(m_pendingDelete, deleteError))
            {
                toForget = m_pendingDelete;
                m_error.clear();
            }
            else
            {
                m_error = deleteError;
            }

            m_pendingDelete.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleColor(4);
        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
        {
            m_pendingDelete.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }

    ImGui::End();

    // Apply changes
    if (pinRequested)
        pm.SetPinned(pinPath, pinValue);

    if (!toForget.empty())
    {
        if (m_selected == toForget)
            m_selected.clear();

        pm.Forget(toForget);
    }

    if (!toOpen.empty())
    {
        const std::string target = NormalizePath(toOpen);
        std::vector<std::string> existingEntries;

        for (const auto& entry : pm.Recent())
        {
            if (NormalizePath(entry.path) == target)
                existingEntries.push_back(entry.path);
        }

        // Remove the existing matching entries
        bool wasPinned = false;
        for (const auto& entry : pm.Recent())
        {
            if (NormalizePath(entry.path) == target && entry.pinned)
                wasPinned = true;
        }

        for (const auto& path : existingEntries)
            pm.Forget(path);

        onOpen(toOpen);

        if (wasPinned && pm.HasProject())
            pm.SetPinned(pm.Current().root.string(), true);
    }
}