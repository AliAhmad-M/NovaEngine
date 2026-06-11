#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <Editor/EditorContext.h>
#include <imgui.h>

enum class AssetType { Unknown, Texture, Scene, Font, Audio };

struct AssetEntry
{
    std::string path;
    std::string filename;

    AssetType type = AssetType::Unknown;
    bool isDirectory = false;
};

class AssetBrowserPanel
{
public:
    explicit AssetBrowserPanel(EditorContext* ctx);

    void Draw();
    void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

    const std::string& GetLastSelectedPath() const { return m_lastSelectedPath; }

private:
    void Refresh();
    void DrawToolbar();
    void DrawDirectoryTree(const std::filesystem::path& root);
    void DrawContentGrid();

    AssetType ClassifyExtension(const std::string& ext) const;
    const char* AssetTypeIcon(AssetType t) const;

    EditorContext* m_ctx = nullptr;
    ImGuiWindowFlags         m_windowFlags = 0;
    std::filesystem::path    m_rootPath;
    std::filesystem::path    m_currentPath;
    std::vector<AssetEntry>  m_entries;
    std::string              m_lastSelectedPath;
    std::string              m_selectedFile;

    float  m_iconSize = 64.0f;
    char   m_filter[128] = {};
};

