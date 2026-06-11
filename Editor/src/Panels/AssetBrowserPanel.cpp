#include <Editor/Panels/AssetBrowserPanel.h>
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <cstdio>

AssetBrowserPanel::AssetBrowserPanel(EditorContext* ctx)
    : m_ctx(ctx)
{
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
    if (ext == ".json" || ext == ".scene") return AssetType::Scene;
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
    std::sort(m_entries.begin(), m_entries.end(), [](const AssetEntry& a, const AssetEntry& b) {
        if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
        return a.filename < b.filename;
        });
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

    std::string label = root.filename().string();
    bool isCurrentDir = (root == m_currentPath);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;
    if (isCurrentDir)
        flags |= ImGuiTreeNodeFlags_Selected;

    // Check if there are sub-directories
    bool hasSubDirs = false;
    for (const auto& e : std::filesystem::directory_iterator(root))
        if (e.is_directory()) { hasSubDirs = true; break; }

    if (!hasSubDirs)
        flags |= ImGuiTreeNodeFlags_Leaf;

    bool open = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
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
        // Apply text filter
        if (m_filter[0] != '\0')
        {
            if (entry.filename.find(m_filter) == std::string::npos)
                continue;
        }

        // Push a unique ID per entry
        ImGui::PushID(entry.path.c_str());

        // Group icon and text
        ImGui::BeginGroup();

        bool isSelected = (m_selectedFile == entry.path);
        ImVec2 iconSize(m_iconSize, m_iconSize);
        ImVec4 bgColor = isSelected
            ? ImVec4(0.26f, 0.50f, 0.84f, 0.50f)
            : ImVec4(0.16f, 0.16f, 0.19f, 1.00f);

        ImGui::PushStyleColor(ImGuiCol_Button, bgColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.50f, 0.84f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.26f, 0.50f, 0.84f, 0.70f));

        // Draw Icon Button
        const char* icon = entry.isDirectory ? "[DIR]" : AssetTypeIcon(entry.type);
        bool iconClicked = ImGui::Button(icon, iconSize);

        ImGui::PopStyleColor(3);

        // Double-click detection
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (!entry.isDirectory)
                m_lastSelectedPath = entry.path;
        }

        // Context menu handler
        if (ImGui::BeginPopupContextItem("##assetCtx"))
        {
            ImGui::TextDisabled("%s", entry.filename.c_str());
            ImGui::Separator();
            if (!entry.isDirectory)
            {
                if (ImGui::MenuItem("Load"))        m_lastSelectedPath = entry.path;
                if (ImGui::MenuItem("Copy Path"))   ImGui::SetClipboardText(entry.path.c_str());
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

        // Tooltip on the icon
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", entry.filename.c_str());

        // Draw filename label
        ImGui::PushItemWidth(iconSize.x);
        std::string truncated = entry.filename;
        if (truncated.size() > 10)
            truncated = truncated.substr(0, 9) + "...";

        // Center text alignment calculation relative to the icon width
        float textWidth = ImGui::CalcTextSize(truncated.c_str()).x;
        if (textWidth < iconSize.x)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (iconSize.x - textWidth) * 0.5f);

        ImGui::TextUnformatted(truncated.c_str());
        ImGui::PopItemWidth();

        if (ImGui::IsItemHovered() && entry.filename.size() > 10)
            ImGui::SetTooltip("%s", entry.filename.c_str());

        ImGui::EndGroup();

        // Handle selection from clicking
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
