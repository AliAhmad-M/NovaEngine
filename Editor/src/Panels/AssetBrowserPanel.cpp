#include <Editor/Panels/AssetBrowserPanel.h>
#include <Core/Services.h>
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <cstdio>

using namespace Nova::Editor;

AssetBrowserPanel::AssetBrowserPanel(Context* ctx)
    : m_ctxRef(ctx)
{
    m_contentLoaderRef = Services::Fetch<ContentLoader>();

    m_iconFolder  = m_contentLoaderRef->LoadTexture("assets/icons/icon_folder.png");
    m_iconTexture = m_contentLoaderRef->LoadTexture("assets/icons/icon_texture.png");
    m_iconScene   = m_contentLoaderRef->LoadTexture("assets/icons/icon_scene.png");
    m_iconAudio   = m_contentLoaderRef->LoadTexture("assets/icons/icon_audio.png");
    m_iconScript  = m_contentLoaderRef->LoadTexture("assets/icons/icon_script.png");
    m_iconFont    = m_contentLoaderRef->LoadTexture("assets/icons/icon_font.png");
    m_iconFolderNew = m_contentLoaderRef->LoadTexture("assets/icons/folder.png");

    // Default to the project's assets folder
    m_rootPath = std::filesystem::path("../Test/project/");
    m_currentPath = m_rootPath;

    // Create the directory if it doesn't exist
    if (!std::filesystem::exists(m_rootPath))
        std::filesystem::create_directories(m_rootPath);

    Refresh();
}

AssetType AssetBrowserPanel::ClassifyExtension(const std::string& ext) const
{
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
        ext == ".bmp" || ext == ".tga")  return AssetType::Texture;
    if (ext == ".jso" || ext == ".nova") return AssetType::Scene;
    if (ext == ".ttf" || ext == ".otf")  return AssetType::Font;
    if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") return AssetType::Audio;
    return AssetType::Unknown;
}

const char* AssetBrowserPanel::AssetTypeIcon(AssetType t) const
{
    switch (t)
    {
    case AssetType::Texture: return "[IMG]";
    case AssetType::Scene:   return "[SCN]";
    case AssetType::Font:    return "[FNT]";
    case AssetType::Audio:   return "[SND]";
    default:                 return "[   ]";
    }
}

GLuint AssetBrowserPanel::GetEntryIcon(const AssetEntry& entry) const
{
    if (entry.isDirectory) 
        return m_iconFolder ? m_iconFolder->GetHandle() : 0;

    switch (entry.type)
    {
        case AssetType::Texture:  
            return m_iconTexture ? m_iconTexture->GetHandle() : 0;
        case AssetType::Scene:    
            return m_iconScene ? m_iconScene->GetHandle() : 0;
        case AssetType::Audio:    
            return m_iconAudio ? m_iconAudio->GetHandle() : 0;
        case AssetType::Font:     
            return m_iconFont ? m_iconFont->GetHandle() : 0;
        default:                  
            return m_iconScript ? m_iconScript->GetHandle() : 0;
    }
}

void AssetBrowserPanel::Refresh()
{
    m_entries.clear();

    if (!std::filesystem::exists(m_currentPath) || !std::filesystem::is_directory(m_currentPath))
        return;

    for (const auto& entry : std::filesystem::directory_iterator(m_currentPath))
    {
        AssetEntry a;
        a.path = entry.path().string();
        a.filename = entry.path().filename().string();
        a.isDirectory = entry.is_directory();

        if (!a.isDirectory)
        {
            std::string ext = entry.path().extension().string();
            // Lowercase the extension for consistent matching
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            a.type = ClassifyExtension(ext);
        }

        m_entries.push_back(std::move(a));
    }

    // Directories first, then alphabetical
    std::sort(m_entries.begin(), m_entries.end(), [](const AssetEntry& a, const AssetEntry& b) 
        {
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.filename < b.filename;
        }
    );
}

void AssetBrowserPanel::Draw()
{
    ImGui::Begin("Asset Browser");

    DrawToolbar();
    ImGui::Separator();

    // Directory file tree and grid
    float leftWidth = 160.0f;
    ImGui::BeginChild("##DirTree", ImVec2(leftWidth, 0), true);
    DrawDirectoryTree(m_rootPath);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##ContentGrid", ImVec2(0, 0), false);
    DrawContentGrid();
    ImGui::EndChild();

    ImGui::End();
}

void AssetBrowserPanel::DrawToolbar()
{
    // Back button
    bool canGoUp = (m_currentPath != m_rootPath);
    if (!canGoUp) ImGui::BeginDisabled();
    if (ImGui::Button("<##back"))
    {
        m_currentPath = m_currentPath.parent_path();
        Refresh();
    }
    if (!canGoUp) ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Go up one directory");

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) Refresh();

    ImGui::SameLine();
    // Breadcrumb
    auto rel = std::filesystem::relative(m_currentPath, m_rootPath.parent_path());
    ImGui::TextDisabled("%s", rel.string().c_str());

    // Search filter
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::InputTextWithHint("##filter", "Filter...", m_filter, sizeof(m_filter));

    // Icon size slider
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::SliderFloat("##iconSize", &m_iconSize, 32.0f, 96.0f, "%.0f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Icon size");
}
void AssetBrowserPanel::DrawDirectoryTree(const std::filesystem::path& root)
{
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) return;

    auto tmp = root;
    if (!tmp.has_filename() || tmp.filename().empty())
        tmp = tmp.parent_path();
    std::string label = tmp.filename().string();
    if (label.empty()) label = tmp.string();
    bool isCurrentDir = (root == m_currentPath);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (isCurrentDir)
        flags |= ImGuiTreeNodeFlags_Selected;

    bool hasSubDirs = false;
    for (const auto& e : std::filesystem::directory_iterator(root))
        if (e.is_directory()) { hasSubDirs = true; break; }

    if (!hasSubDirs)
        flags |= ImGuiTreeNodeFlags_Leaf;

    // Draw arrow only with hidden label
    std::string hiddenLabel = "##tree_" + label;
    bool open = ImGui::TreeNodeEx(hiddenLabel.c_str(), flags);

    // Draw icon and label on same line after arrow
    ImGui::SameLine();

    if (m_iconFolderNew && m_iconFolderNew->GetHandle())
    {
        ImGui::Image(
            (ImTextureID)(intptr_t)m_iconFolderNew->GetHandle(),
            ImVec2(16, 16),
            ImVec2(0, 0),
            ImVec2(1, 1)
        );
        ImGui::SameLine();
    }

    // Selectable as the clickable label
    bool clicked = ImGui::Selectable(label.c_str(), isCurrentDir,
        ImGuiSelectableFlags_None, ImVec2(0, 16));

    if (clicked)
    {
        m_currentPath = root;
        Refresh();
    }

    if (open)
    {
        for (const auto& e : std::filesystem::directory_iterator(root))
            if (e.is_directory())
                DrawDirectoryTree(e.path());
        ImGui::TreePop();
    }
}
void AssetBrowserPanel::DrawContentGrid()
{
    float cellSize = m_iconSize + 16.0f;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int   cols = std::max(1, (int)(panelWidth / cellSize));

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 12));

    int column = 0;
    for (const auto& entry : m_entries)
    {
        if (m_filter[0] != '\0')
        {
            if (entry.filename.find(m_filter) == std::string::npos)
                continue;
        }

        GLuint iconTex = GetEntryIcon(entry);
        if (iconTex == 0) continue;  // Skip rendering if no valid texture

        ImGui::PushID(entry.path.c_str());
        ImGui::BeginGroup();

        bool isSelected = (m_selectedFile == entry.path);
        ImVec2 iconSize(m_iconSize, m_iconSize);
        ImVec4 bgColor = isSelected
            ? ImVec4(0.26f, 0.50f, 0.84f, 0.50f)
            : ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // transparent when not selected

        // only 3 PushStyleColor calls, not 6
        ImGui::PushStyleColor(ImGuiCol_Button, bgColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.50f, 0.84f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.26f, 0.50f, 0.84f, 0.70f));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        bool iconClicked = ImGui::ImageButton(
            "##icon",
            (ImTextureID)(intptr_t)iconTex,
            ImVec2(m_iconSize, m_iconSize),
            ImVec2(0, 0),           // uv0
            ImVec2(1, 1),           // uv1
            ImVec4(0, 0, 0, 0),     // background -> transparent
            ImVec4(1, 1, 1, 1)      // tint -> white
        );
        ImGui::PopStyleVar();

        ImGui::PopStyleColor(3);

        // Double-click detection
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (!entry.isDirectory)
                m_lastSelectedPath = entry.path;
        }

        // Context menu
        if (ImGui::BeginPopupContextItem("##assetCtx"))
        {
            ImGui::TextDisabled("%s", entry.filename.c_str());
            ImGui::Separator();
            if (!entry.isDirectory)
            {
                if (ImGui::MenuItem("Load"))       m_lastSelectedPath = entry.path;
                if (ImGui::MenuItem("Copy Path"))  ImGui::SetClipboardText(entry.path.c_str());
            }
            if (ImGui::MenuItem("Show in Explorer"))
            {
#ifdef _WIN32
                std::string cmd = "explorer /select,\"" + entry.path + "\"";
                std::system(cmd.c_str());
#endif
            }
            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", entry.filename.c_str());

        // Filename label
        std::string truncated = entry.filename;
        if (truncated.size() > 10)
            truncated = truncated.substr(0, 9) + "...";

        float textWidth = ImGui::CalcTextSize(truncated.c_str()).x;
        if (textWidth < iconSize.x)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (iconSize.x - textWidth) * 0.5f);

        ImGui::TextUnformatted(truncated.c_str());

        if (ImGui::IsItemHovered() && entry.filename.size() > 10)
            ImGui::SetTooltip("%s", entry.filename.c_str());

        ImGui::EndGroup();

        if (iconClicked || (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)))
        {
            m_selectedFile = entry.path;
            if (iconClicked && entry.isDirectory)
            {
                m_currentPath = std::filesystem::path(entry.path);
                Refresh();
                ImGui::PopID();
                break;
            }
        }

        ImGui::PopID();

        ++column;
        if (column < cols)
            ImGui::SameLine();
        else
            column = 0;
    }

    ImGui::PopStyleVar();
}