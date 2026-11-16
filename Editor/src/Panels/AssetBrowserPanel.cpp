#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Panels/AssetBrowserPanel.h>
#include <Editor/Panels/ScenePanel.h>
#include <Editor/FilePicker.h>
#include <Editor/AssetPath.h>
#include <Editor/Scene/SceneSerializer.h>
#include <Core/Services.h>
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <fstream>

using namespace Nova::Editor;

AssetBrowserPanel::AssetBrowserPanel(EditorContext* ctx)
    : m_ctxRef(ctx)
{
    m_contentLoaderRef = Services::Fetch<ContentLoader>();

    m_iconFolder    = m_contentLoaderRef->LoadTexture("assets/icons/icon_folder.png");
    m_iconTexture   = m_contentLoaderRef->LoadTexture("assets/icons/icon_texture.png");
    m_iconScene     = m_contentLoaderRef->LoadTexture("assets/icons/icon_scene.png");
    m_iconAudio     = m_contentLoaderRef->LoadTexture("assets/icons/icon_audio.png");
    m_iconScript    = m_contentLoaderRef->LoadTexture("assets/icons/icon_script.png");
    m_iconFont      = m_contentLoaderRef->LoadTexture("assets/icons/icon_font.png");
    m_iconFolderNew = m_contentLoaderRef->LoadTexture("assets/icons/folder.png");
}

AssetType AssetBrowserPanel::classifyExtension(const std::string& ext) const
{
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
        ext == ".bmp" || ext == ".tga")  return AssetType::Texture;
    if (ext == ".jso" || ext == ".nvsn") return AssetType::Scene;
    if (ext == ".ttf" || ext == ".otf")  return AssetType::Font;
    if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") return AssetType::Audio;
    if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".inl") return AssetType::Script;
    return AssetType::Unknown;
}

const char* AssetBrowserPanel::assetTypeLabel(AssetType t, bool isDir) const
{
    if (isDir) return "FOLDER";
    switch (t)
    {
    case AssetType::Texture: return "TEXTURE";
    case AssetType::Scene:   return "SCENE";
    case AssetType::Font:    return "FONT";
    case AssetType::Audio:   return "AUDIO";
    case AssetType::Script:  return "SCRIPT";
    default:                 return "FILE";
    }
}

ImVec4 AssetBrowserPanel::assetTypeColor(AssetType t, bool isDir) const
{
    if (isDir) return ImVec4(0.92f, 0.72f, 0.20f, 1.0f); // Amber / Yellow
    switch (t)
    {
    case AssetType::Texture: return ImVec4(0.15f, 0.55f, 0.95f, 1.0f); // Blue
    case AssetType::Scene:   return ImVec4(0.95f, 0.50f, 0.15f, 1.0f); // Orange
    case AssetType::Font:    return ImVec4(0.15f, 0.80f, 0.85f, 1.0f); // Cyan
    case AssetType::Audio:   return ImVec4(0.85f, 0.25f, 0.65f, 1.0f); // Magenta
    case AssetType::Script:  return ImVec4(0.25f, 0.75f, 0.35f, 1.0f); // Green
    default:                 return ImVec4(0.50f, 0.52f, 0.58f, 1.0f); // Gray
    }
}

GLuint AssetBrowserPanel::getEntryIcon(const AssetEntry& entry) const
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
        case AssetType::Script:
            return m_iconScript ? m_iconScript->GetHandle() : 0;
        default:                  
            return m_iconScript ? m_iconScript->GetHandle() : 0;
    }
}

void AssetBrowserPanel::refresh()
{
    m_entries.clear();

    if (!std::filesystem::exists(m_currentPath) || !std::filesystem::is_directory(m_currentPath))
        return;

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(m_currentPath, ec))
    {
        if (ec) continue;

        AssetEntry a;
        a.path = entry.path().string();
        a.filename = entry.path().filename().string();
        a.isDirectory = entry.is_directory(ec);

        if (!a.isDirectory)
        {
            a.fileSize = entry.file_size(ec);
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            a.type = classifyExtension(ext);
        }

        m_entries.push_back(std::move(a));
    }

    // Directories first, then alphabetical
    std::sort(m_entries.begin(), m_entries.end(), [](const AssetEntry& a, const AssetEntry& b) 
    {
        if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
        return a.filename < b.filename;
    });
}

void AssetBrowserPanel::SetRoot(const std::filesystem::path& root)
{
    m_rootPath = root;
    m_currentPath = root;
    m_selectedFile.clear();
    refresh();
}

void AssetBrowserPanel::importAsset()
{
    std::string src = OpenNativeFileDialog("Import Asset into Project", "All Files (*.*)", "*.*");
    if (!src.empty())
    {
        std::filesystem::path srcPath(src);
        std::filesystem::path dstPath = m_currentPath / srcPath.filename();
        std::error_code ec;
        std::filesystem::copy_file(srcPath, dstPath, std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec)
        {
            refresh();
            m_selectedFile = dstPath.string();
        }
    }
}

void AssetBrowserPanel::createNewFolder(const std::string& name)
{
    std::string safeName = name.empty() ? "NewFolder" : name;
    std::filesystem::path target = m_currentPath / safeName;
    std::error_code ec;

    // Avoid collision
    int suffix = 1;
    while (std::filesystem::exists(target, ec))
    {
        target = m_currentPath / (safeName + "_" + std::to_string(suffix++));
    }

    std::filesystem::create_directories(target, ec);
    refresh();
    m_selectedFile = target.string();
}

void AssetBrowserPanel::createNewScene(const std::string& name)
{
    std::string safeName = name.empty() ? "NewScene" : name;
    std::filesystem::path target = m_currentPath / (safeName + ".nvsn");
    std::error_code ec;

    int suffix = 1;
    while (std::filesystem::exists(target, ec))
    {
        target = m_currentPath / (safeName + "_" + std::to_string(suffix++) + ".nvsn");
    }

    Scene newScene(target.stem().string());
    newScene.SetFilePath(target.string());
    SceneSerializer serializer;
    serializer.Save(newScene, target.string());

    refresh();
    m_selectedFile = target.string();

    if (m_ctxRef && m_ctxRef->scenePanelRef)
        m_ctxRef->scenePanelRef->OpenSceneTab(target.string());
}

void AssetBrowserPanel::Draw()
{
    ImGui::Begin("Asset Browser");

    drawToolbar();
    drawBreadcrumbs();
    ImGui::Separator();

    // Keyboard shortcuts
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !m_selectedFile.empty())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            m_itemToDelete = m_selectedFile;
            m_showDeleteModal = true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_F2))
        {
            m_itemToRename = m_selectedFile;
            std::string fn = std::filesystem::path(m_selectedFile).filename().string();
            std::strncpy(m_renameBuf, fn.c_str(), sizeof(m_renameBuf) - 1);
            m_showRenameModal = true;
        }
    }

    // Directory file tree and grid
    float leftWidth = 180.0f;
    ImGui::BeginChild("##DirTree", ImVec2(leftWidth, -26.0f), true);
    drawDirectoryTree(m_rootPath);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##ContentGrid", ImVec2(0, -26.0f), false);
    drawContentGrid();
    ImGui::EndChild();

    drawStatusBar();
    drawModals();

    ImGui::End();
}

void AssetBrowserPanel::drawToolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));

    // Back / Up button
    bool canGoUp = (m_currentPath != m_rootPath && m_currentPath.has_parent_path());
    if (!canGoUp) ImGui::BeginDisabled();
    if (ImGui::Button(" < "))
    {
        m_currentPath = m_currentPath.parent_path();
        refresh();
    }
    if (!canGoUp) ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Go up one directory");

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) refresh();

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Import Asset
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.45f, 0.94f, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.45f, 0.94f, 0.50f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.45f, 0.94f, 0.80f));
    if (ImGui::Button("+ Import"))
    {
        importAsset();
    }
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Import an external file into this folder using OS dialog");

    ImGui::SameLine();
    if (ImGui::Button("+ Folder"))
    {
        m_showNewFolderModal = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("+ Scene"))
    {
        m_showNewSceneModal = true;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Search filter
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputTextWithHint("##filter", "Filter assets...", m_filter, sizeof(m_filter));

    // Icon size slider
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);
    ImGui::SliderFloat("##iconSize", &m_iconSize, 40.0f, 96.0f, "%.0f px");
    if (ImGui::IsItemHovered() && !ImGui::GetDragDropPayload()) ImGui::SetTooltip("Thumbnail Size");

    ImGui::PopStyleVar();
}

void AssetBrowserPanel::drawBreadcrumbs()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.25f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.8f, 0.5f));

    std::error_code ec;
    auto rel = std::filesystem::relative(m_currentPath, m_rootPath, ec);

    // Root button
    if (ImGui::SmallButton("Content"))
    {
        m_currentPath = m_rootPath;
        refresh();
    }

    if (!ec && rel != "." && !rel.empty())
    {
        std::filesystem::path accum = m_rootPath;
        for (const auto& part : rel)
        {
            accum /= part;
            ImGui::SameLine(0, 3);
            ImGui::TextDisabled(">");
            ImGui::SameLine(0, 3);
            if (ImGui::SmallButton(part.string().c_str()))
            {
                m_currentPath = accum;
                refresh();
                break;
            }
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
}

void AssetBrowserPanel::drawDirectoryTree(const std::filesystem::path& root)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec)) return;

    auto tmp = root;
    if (!tmp.has_filename())
        tmp = tmp.parent_path();
    std::string label = tmp.filename().string();
    if (label.empty()) label = "Content";

    std::vector<std::filesystem::path> subDirs;
    for (const auto& e : std::filesystem::directory_iterator(root, ec))
        if (e.is_directory(ec))
            subDirs.push_back(e.path());
    std::sort(subDirs.begin(), subDirs.end());

    constexpr float rowH = 24.0f;
    constexpr float iconSize = 16.0f;
    constexpr float iconGap = 6.0f;

    bool isCurrentDir = (root == m_currentPath);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_FramePadding;
    if (isCurrentDir)     flags |= ImGuiTreeNodeFlags_Selected;
    if (subDirs.empty())  flags |= ImGuiTreeNodeFlags_Leaf;

    ImGui::PushID(root.string().c_str());

    float padY = (rowH - ImGui::GetFontSize()) * 0.5f;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
        ImVec2(ImGui::GetStyle().FramePadding.x, padY));
    bool open = ImGui::TreeNodeEx("##node", flags);
    ImGui::PopStyleVar();

    ImVec2 rMin = ImGui::GetItemRectMin();
    ImVec2 rMax = ImGui::GetItemRectMax();

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
    {
        m_currentPath = root;
        refresh();
    }

    // Context menu on directory tree nodes
    if (ImGui::BeginPopupContextItem("##treeNodeCtx"))
    {
        ImGui::TextDisabled("%s", label.c_str());
        ImGui::Separator();
        if (ImGui::MenuItem("+ New Folder"))
        {
            m_currentPath = root;
            m_showNewFolderModal = true;
        }
        if (ImGui::MenuItem("Show in Explorer"))
        {
#ifdef _WIN32
            std::string cmd = "explorer /select,\"" + root.string() + "\"";
            std::system(cmd.c_str());
#endif
        }
        ImGui::EndPopup();
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    float cy = (rMin.y + rMax.y) * 0.5f;
    float x = rMin.x + ImGui::GetTreeNodeToLabelSpacing();

    if (m_iconFolderNew && m_iconFolderNew->GetHandle())
    {
        dl->AddImage(
            (ImTextureID)(intptr_t)m_iconFolderNew->GetHandle(),
            ImVec2(x, cy - iconSize * 0.5f),
            ImVec2(x + iconSize, cy + iconSize * 0.5f));
        x += iconSize + iconGap;
    }

    dl->AddText(ImVec2(x, cy - ImGui::GetFontSize() * 0.5f),
        ImGui::GetColorU32(ImGuiCol_Text), label.c_str());

    if (open)
    {
        for (const auto& sub : subDirs)
            drawDirectoryTree(sub);
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void AssetBrowserPanel::drawContentGrid()
{
    constexpr float kPad = 4.0f;
    constexpr float kSpacing = 10.0f;
    const float cardW = m_iconSize + kPad * 2.0f;
    const float panelW = ImGui::GetContentRegionAvail().x;
    int cols = std::max(1, (int)((panelW + kSpacing) / (cardW + kSpacing)));

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kSpacing, kSpacing));

    int column = 0;
    for (const auto& entry : m_entries)
    {
        if (m_filter[0] != '\0')
        {
            std::string fLower = m_filter;
            std::string eLower = entry.filename;
            std::transform(fLower.begin(), fLower.end(), fLower.begin(), ::tolower);
            std::transform(eLower.begin(), eLower.end(), eLower.begin(), ::tolower);
            if (eLower.find(fLower) == std::string::npos)
                continue;
        }

        GLuint iconTex = getEntryIcon(entry);
        if (iconTex == 0) continue;

        ImGui::PushID(entry.path.c_str());
        ImGui::BeginGroup();
        float startX = ImGui::GetCursorPosX();

        bool isSelected = (m_selectedFile == entry.path);
        ImVec4 tagColor = assetTypeColor(entry.type, entry.isDirectory);

        // Card button styling
        ImVec4 bgColor = isSelected
            ? ImVec4(0.0f, 0.45f, 0.94f, 0.35f)
            : ImVec4(0.12f, 0.13f, 0.16f, 0.60f);

        ImGui::PushStyleColor(ImGuiCol_Button, bgColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.45f, 0.94f, 0.25f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.45f, 0.94f, 0.60f));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        bool iconClicked = ImGui::ImageButton(
            "##icon",
            (ImTextureID)(intptr_t)iconTex,
            ImVec2(m_iconSize, m_iconSize),
            ImVec2(0, 0),
            ImVec2(1, 1),
            ImVec4(0, 0, 0, 0),
            ImVec4(1, 1, 1, 1)
        );
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImVec2 cardMin = ImGui::GetItemRectMin();
        ImVec2 cardMax = ImGui::GetItemRectMax();

        // Colored stripe
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(
            ImVec2(cardMin.x + 2.0f, cardMax.y - 3.0f),
            ImVec2(cardMax.x - 2.0f, cardMax.y),
            ImColor(tagColor),
            1.5f);

        if (!entry.isDirectory && ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(kAssetPayload, entry.path.c_str(), entry.path.size() + 1);
            ImGui::Image((ImTextureID)(intptr_t)iconTex, ImVec2(32, 32));
            ImGui::SameLine();
            ImGui::TextUnformatted(entry.filename.c_str());
            ImGui::EndDragDropSource();
        }

        // Detect double click
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (entry.isDirectory)
            {
                m_currentPath = std::filesystem::path(entry.path);
                refresh();
                ImGui::EndGroup();
                ImGui::PopID();
                break;
            }
            else
            {
                m_lastSelectedPath = entry.path;
                if (entry.type == AssetType::Scene && m_ctxRef && m_ctxRef->scenePanelRef)
                {
                    m_ctxRef->scenePanelRef->OpenSceneTab(entry.path);
                }
            }
        }

        // Context menu for each asset
        if (ImGui::BeginPopupContextItem("##assetCtx"))
        {
            ImGui::TextDisabled("%s", entry.filename.c_str());
            ImGui::Separator();

            if (entry.isDirectory)
            {
                if (ImGui::MenuItem("Open Folder"))
                {
                    m_currentPath = std::filesystem::path(entry.path);
                    refresh();
                }
            }
            else
            {
                if (entry.type == AssetType::Scene && ImGui::MenuItem("Open in Scene Editor"))
                {
                    if (m_ctxRef && m_ctxRef->scenePanelRef)
                        m_ctxRef->scenePanelRef->OpenSceneTab(entry.path);
                }
            }

            if (ImGui::MenuItem("Show in Explorer"))
            {
#ifdef _WIN32
                std::string cmd = "explorer /select,\"" + entry.path + "\"";
                std::system(cmd.c_str());
#endif
            }

            if (ImGui::MenuItem("Rename", "F2"))
            {
                m_itemToRename = entry.path;
                std::strncpy(m_renameBuf, entry.filename.c_str(), sizeof(m_renameBuf) - 1);
                m_showRenameModal = true;
            }

            if (ImGui::MenuItem("Copy Full Path"))
            {
                ImGui::SetClipboardText(entry.path.c_str());
            }

            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.35f, 0.35f, 1.0f));
            if (ImGui::MenuItem("Delete", "Del"))
            {
                m_itemToDelete = entry.path;
                m_showDeleteModal = true;
            }
            ImGui::PopStyleColor();

            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s\nType: %s", entry.filename.c_str(), assetTypeLabel(entry.type, entry.isDirectory));

        // Filename label centered under thumbnail
        std::string label = entry.filename;
        if (ImGui::CalcTextSize(label.c_str()).x > cardW)
        {
            const float ellW = ImGui::CalcTextSize("...").x;
            size_t n = label.size();
            while (n > 0)
            {
                --n;
                while (n > 0 && (label[n] & 0xC0) == 0x80) --n;  // don't split a UTF-8 sequence
                if (ImGui::CalcTextSize(label.c_str(), label.c_str() + n).x + ellW <= cardW) break;
            }
            label = label.substr(0, n) + "...";
        }

        float textW = ImGui::CalcTextSize(label.c_str()).x;
        ImGui::SetCursorPosX(startX + (cardW - textW) * 0.5f);
        ImGui::TextUnformatted(label.c_str());

        if (ImGui::IsItemHovered() && label != entry.filename)
            ImGui::SetTooltip("%s", entry.filename.c_str());

        ImGui::EndGroup();

        if (iconClicked || (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)))
        {
            m_selectedFile = entry.path;
        }

        ImGui::PopID();

        column++;
        if (column < cols)
            ImGui::SameLine();
        else
            column = 0;
    }

    // Right click on blank background of content grid
    if (ImGui::BeginPopupContextWindow("##GridBgCtx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("+ New Folder")) m_showNewFolderModal = true;
        if (ImGui::MenuItem("+ New Scene"))  m_showNewSceneModal = true;
        if (ImGui::MenuItem("Import Asset...")) importAsset();
        ImGui::Separator();
        if (ImGui::MenuItem("Show in Explorer"))
        {
#ifdef _WIN32
            std::string cmd = "explorer \"" + m_currentPath.string() + "\"";
            std::system(cmd.c_str());
#endif
        }
        if (ImGui::MenuItem("Refresh")) refresh();
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
}

void AssetBrowserPanel::drawStatusBar()
{
    ImGui::Separator();

    std::string status = std::to_string(m_entries.size()) + " items";

    if (!m_selectedFile.empty())
    {
        std::filesystem::path p(m_selectedFile);
        status += "  |  Selected: " + p.filename().string();
        std::error_code ec;
        if (std::filesystem::is_regular_file(p, ec))
        {
            auto sizeBytes = std::filesystem::file_size(p, ec);
            if (!ec)
            {
                char sizeBuf[64];
                if (sizeBytes >= 1024 * 1024)
                    std::snprintf(sizeBuf, sizeof(sizeBuf), " (%.1f MB)", sizeBytes / (1024.0f * 1024.0f));
                else if (sizeBytes >= 1024)
                    std::snprintf(sizeBuf, sizeof(sizeBuf), " (%.1f KB)", sizeBytes / 1024.0f);
                else
                    std::snprintf(sizeBuf, sizeof(sizeBuf), " (%llu B)", (unsigned long long)sizeBytes);
                status += sizeBuf;
            }
        }
    }

    ImGui::TextDisabled("%s", status.c_str());
}

void AssetBrowserPanel::drawModals()
{
    // Delete Confirmation Modal
    if (m_showDeleteModal)
    {
        ImGui::OpenPopup("Delete Asset?##modal");
        m_showDeleteModal = false;
    }

    if (ImGui::BeginPopupModal("Delete Asset?##modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        std::filesystem::path p(m_itemToDelete);
        ImGui::Text("Are you sure you want to permanently delete:\n\"%s\"?", p.filename().string().c_str());
        ImGui::Spacing();
        ImGui::TextDisabled("This operation cannot be undone.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("Delete", ImVec2(100, 0)))
        {
            std::error_code ec;
            std::filesystem::remove_all(m_itemToDelete, ec);
            if (m_selectedFile == m_itemToDelete)
                m_selectedFile.clear();
            m_itemToDelete.clear();
            refresh();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0)))
        {
            m_itemToDelete.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Rename Modal
    if (m_showRenameModal)
    {
        ImGui::OpenPopup("Rename Asset##modal");
        m_showRenameModal = false;
    }

    if (ImGui::BeginPopupModal("Rename Asset##modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter new name:");
        ImGui::SetNextItemWidth(300.0f);
        bool enterPressed = ImGui::InputText("##renameInput", m_renameBuf, sizeof(m_renameBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (enterPressed || ImGui::Button("Rename", ImVec2(100, 0)))
        {
            if (m_renameBuf[0] != '\0')
            {
                std::filesystem::path oldP(m_itemToRename);
                std::filesystem::path newP = oldP.parent_path() / m_renameBuf;
                std::error_code ec;
                std::filesystem::rename(oldP, newP, ec);
                if (!ec)
                {
                    m_selectedFile = newP.string();
                    refresh();
                }
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // New Folder Modal
    if (m_showNewFolderModal)
    {
        ImGui::OpenPopup("New Folder##modal");
        m_showNewFolderModal = false;
    }

    if (ImGui::BeginPopupModal("New Folder##modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Folder name:");
        ImGui::SetNextItemWidth(260.0f);
        bool enterPressed = ImGui::InputText("##folderInput", m_newFolderBuf, sizeof(m_newFolderBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (enterPressed || ImGui::Button("Create", ImVec2(100, 0)))
        {
            createNewFolder(m_newFolderBuf);
            std::strncpy(m_newFolderBuf, "NewFolder", sizeof(m_newFolderBuf));
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // New Scene Modal
    if (m_showNewSceneModal)
    {
        ImGui::OpenPopup("New Scene##modal");
        m_showNewSceneModal = false;
    }

    if (ImGui::BeginPopupModal("New Scene##modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Scene name:");
        ImGui::SetNextItemWidth(260.0f);
        bool enterPressed = ImGui::InputText("##sceneInput", m_newSceneBuf, sizeof(m_newSceneBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (enterPressed || ImGui::Button("Create", ImVec2(100, 0)))
        {
            createNewScene(m_newSceneBuf);
            std::strncpy(m_newSceneBuf, "NewScene", sizeof(m_newSceneBuf));
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}