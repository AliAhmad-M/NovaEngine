#include <Editor/FilePicker.h>
#include <imgui.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cstring>
#include <unordered_map>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

std::string Nova::Editor::OpenNativeFileDialog(
    const char* title,
    const char* filterName,
    const char* filterSpec,
    const std::string& defaultPath)
{
    std::string result;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IFileOpenDialog* pFileOpen = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                   IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen))))
    {
        if (title)
        {
            std::wstring wtitle = std::filesystem::path(title).wstring();
            pFileOpen->SetTitle(wtitle.c_str());
        }

        if (filterName && filterSpec)
        {
            std::wstring wName = std::filesystem::path(filterName).wstring();
            std::wstring wSpec = std::filesystem::path(filterSpec).wstring();
            COMDLG_FILTERSPEC spec[] = { { wName.c_str(), wSpec.c_str() } };
            pFileOpen->SetFileTypes(1, spec);
        }

        if (!defaultPath.empty())
        {
            std::wstring wpath = std::filesystem::path(defaultPath).wstring();
            IShellItem* pFolder = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(wpath.c_str(), NULL, IID_PPV_ARGS(&pFolder))))
            {
                pFileOpen->SetFolder(pFolder);
                pFolder->Release();
            }
        }

        if (SUCCEEDED(pFileOpen->Show(NULL)))
        {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pFileOpen->GetResult(&pItem)))
            {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
                {
                    result = std::filesystem::path(pszFilePath).string();
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }

    CoUninitialize();
    return result;
}

std::string Nova::Editor::SaveNativeFileDialog(
    const char* title,
    const char* filterName,
    const char* filterSpec,
    const std::string& defaultPath,
    const std::string& defaultName)
{
    std::string result;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IFileSaveDialog* pFileSave = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
                                   IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave))))
    {
        if (title)
        {
            std::wstring wtitle = std::filesystem::path(title).wstring();
            pFileSave->SetTitle(wtitle.c_str());
        }

        if (filterName && filterSpec)
        {
            std::wstring wName = std::filesystem::path(filterName).wstring();
            std::wstring wSpec = std::filesystem::path(filterSpec).wstring();
            COMDLG_FILTERSPEC spec[] = { { wName.c_str(), wSpec.c_str() } };
            pFileSave->SetFileTypes(1, spec);
        }

        if (!defaultName.empty())
        {
            std::wstring wname = std::filesystem::path(defaultName).wstring();
            pFileSave->SetFileName(wname.c_str());
        }

        if (!defaultPath.empty())
        {
            std::wstring wpath = std::filesystem::path(defaultPath).wstring();
            IShellItem* pFolder = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(wpath.c_str(), NULL, IID_PPV_ARGS(&pFolder))))
            {
                pFileSave->SetFolder(pFolder);
                pFolder->Release();
            }
        }

        if (SUCCEEDED(pFileSave->Show(NULL)))
        {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pFileSave->GetResult(&pItem)))
            {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
                {
                    result = std::filesystem::path(pszFilePath).string();
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileSave->Release();
    }

    CoUninitialize();
    return result;
}

std::string Nova::Editor::OpenNativeFolderDialog(
    const char* title,
    const std::string& defaultPath)
{
    std::string result;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IFileOpenDialog* pFolderOpen = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                   IID_IFileOpenDialog, reinterpret_cast<void**>(&pFolderOpen))))
    {
        pFolderOpen->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);

        if (title)
        {
            std::wstring wtitle = std::filesystem::path(title).wstring();
            pFolderOpen->SetTitle(wtitle.c_str());
        }

        if (!defaultPath.empty())
        {
            std::wstring wpath = std::filesystem::path(defaultPath).wstring();
            IShellItem* pFolder = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(wpath.c_str(), NULL, IID_PPV_ARGS(&pFolder))))
            {
                pFolderOpen->SetFolder(pFolder);
                pFolder->Release();
            }
        }

        if (SUCCEEDED(pFolderOpen->Show(NULL)))
        {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pFolderOpen->GetResult(&pItem)))
            {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
                {
                    result = std::filesystem::path(pszFilePath).string();
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFolderOpen->Release();
    }

    CoUninitialize();
    return result;
}
#else
std::string Nova::Editor::OpenNativeFileDialog(const char*, const char*, const char*, const std::string&) { return ""; }
std::string Nova::Editor::SaveNativeFileDialog(const char*, const char*, const char*, const std::string&, const std::string&) { return ""; }
std::string Nova::Editor::OpenNativeFolderDialog(const char*, const std::string&) { return ""; }
#endif

namespace fs = std::filesystem;
using namespace Nova::Editor;

bool Nova::Editor::DrawFilePicker(
    const char* id,
    std::string& currentPath,
    const char* filterExt,
    std::function<void(const std::string&)> onConfirm)
{
    ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);

    bool open = true;
    if (!ImGui::BeginPopupModal(id, &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize))
        return false;

    // Per-picker state keyed by id
    struct State
    {
        fs::path    dir;
        std::string selected;
        char        inputBuf[512] = {};
        bool        initialized = false;
    };
    static std::unordered_map<std::string, State> s_states;
    State& s = s_states[id];

    bool folderMode = (!filterExt || filterExt[0] == '\0');

    if (ImGui::IsWindowAppearing())
    {
        s.selected.clear();
        std::memset(s.inputBuf, 0, sizeof(s.inputBuf));

        if (!currentPath.empty())
        {
            std::error_code ec;
            fs::path p(currentPath);

            if (fs::is_directory(p, ec) && !ec)
            {
                s.dir = p;
                if (folderMode)
                    s.selected = p.string();
            }
            else if (fs::is_regular_file(p, ec) && !ec)
            {
                s.dir = p.parent_path();
                s.selected = p.string();
            }
            else
            {
                fs::path parent = p.parent_path();
                if (fs::is_directory(parent, ec) && !ec)
                {
                    s.dir = parent;
                    if (folderMode)
                        s.selected = parent.string();
                }
                else
                {
                    s.dir = fs::current_path();
                    if (folderMode)
                        s.selected = s.dir.string();
                }
            }
        }
        else if (s.dir.empty())
        {
            s.dir = fs::current_path();
            if (folderMode)
                s.selected = s.dir.string();
        }

        if (!s.selected.empty())
            std::snprintf(s.inputBuf, sizeof(s.inputBuf), "%s", s.selected.c_str());

        s.initialized = true;
    }

    if (!s.initialized)
    {
        s.dir = fs::current_path();
        if (folderMode)
        {
            s.selected = s.dir.string();
            std::snprintf(s.inputBuf, sizeof(s.inputBuf), "%s", s.selected.c_str());
        }
        s.initialized = true;
    }

    // Nav bar
    const float navHeight = 32.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::BeginChild("##nav", ImVec2(0, navHeight), false, ImGuiWindowFlags_NoScrollbar);

    // Up button
    ImGui::SetCursorPosY((navHeight - ImGui::GetFrameHeight()) * 0.5f);
    ImGui::BeginDisabled(!s.dir.has_parent_path());
    if (ImGui::Button(" ^ "))
    {
        s.dir = s.dir.parent_path();
        s.selected = folderMode ? s.dir.string() : "";
        std::snprintf(s.inputBuf, sizeof(s.inputBuf), "%s", s.selected.c_str());
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0, 8);

    // Windows Explorer native dialog trigger
    if (ImGui::Button("OS Dialog..."))
    {
        std::string chosen;
        if (folderMode)
        {
            chosen = OpenNativeFolderDialog("Select Folder", s.dir.string());
        }
        else
        {
            std::string filterName = filterExt ? std::string(filterExt) + " Files" : "All Files";
            std::string filterSpec = filterExt ? std::string("*") + filterExt : "*.*";
            chosen = OpenNativeFileDialog("Select File", filterName.c_str(), filterSpec.c_str(), s.dir.string());
        }

        if (!chosen.empty())
        {
            onConfirm(chosen);
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return false;
        }
    }

    ImGui::SameLine(0, 8);

    // Path display
    ImGui::SetCursorPosY((navHeight - ImGui::GetFrameHeight()) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextUnformatted(s.dir.string().c_str());
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // File list
    const float bottomBarH = 44.0f + ImGui::GetStyle().ItemSpacing.y * 2.0f;
    ImGui::BeginChild("##files",
        ImVec2(0, ImGui::GetContentRegionAvail().y - bottomBarH),
        true);

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
            else if (!folderMode && entry.is_regular_file(entryEc) && !entryEc)
            {
                if (filterExt && filterExt[0] != '\0'
                    && entry.path().extension() != filterExt) continue;
                files.push_back(entry);
            }
        }
    }
    catch (const fs::filesystem_error&)
    {
        if (s.dir.has_parent_path())
            s.dir = s.dir.parent_path();
    }
    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());

    const ImVec4 folderCol = ImVec4(0.95f, 0.75f, 0.20f, 1.00f);
    const ImVec4 fileCol = ImGui::GetStyleColorVec4(ImGuiCol_Text);

    for (auto& d : dirs)
    {
        bool isSelected = (s.selected == d.path().string());

        ImGui::PushStyleColor(ImGuiCol_Text, folderCol);
        if (ImGui::Selectable(("  [/] " + d.path().filename().string()).c_str(),
            isSelected,
            ImGuiSelectableFlags_AllowDoubleClick))
        {
            if (ImGui::IsMouseDoubleClicked(0))
            {
                s.dir = d.path();
                s.selected = folderMode ? s.dir.string() : "";
                std::snprintf(s.inputBuf, sizeof(s.inputBuf), "%s", s.selected.c_str());
            }
            else
            {
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

    const float btnW = 80.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (btnW + spacing) * 2 - spacing);

    bool enterPressed = ImGui::InputText("##path", s.inputBuf, sizeof(s.inputBuf), ImGuiInputTextFlags_EnterReturnsTrue);
    if (!enterPressed && ImGui::IsItemDeactivatedAfterEdit())
        s.selected = s.inputBuf;

    ImGui::SameLine();

    const char* confirmLabel = folderMode ? "Select" : "Open";

    ImGui::BeginDisabled(s.selected.empty() && !folderMode);
    if (enterPressed || ImGui::Button(confirmLabel, ImVec2(btnW, 0)))
    {
        std::string result = s.selected.empty() ? s.dir.string() : s.selected;
        onConfirm(result);
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
