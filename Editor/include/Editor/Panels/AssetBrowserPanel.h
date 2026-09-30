#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <Editor/Context.h>
#include <Graphics/Texture.h>
#include <Runtime/ContentLoader.h>
#include <imgui.h>

namespace Nova::Editor 
{
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
        explicit AssetBrowserPanel(Context* ctx);

        void Draw();
        void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

        const std::string& GetLastSelectedPath() const { return m_lastSelectedPath; }

    private:
        void Refresh();
        void DrawToolbar();
        void DrawDirectoryTree(const std::filesystem::path& root);
        void DrawContentGrid();

        AssetType   ClassifyExtension(const std::string& ext) const;
        const char* AssetTypeIcon(AssetType t) const;
        GLuint      GetEntryIcon(const AssetEntry& entry) const;

        Context* m_ctxRef = nullptr;
        ImGuiWindowFlags         m_windowFlags = 0;
        std::filesystem::path    m_rootPath;
        std::filesystem::path    m_currentPath;
        std::vector<AssetEntry>  m_entries;
        std::string              m_lastSelectedPath;
        std::string              m_selectedFile;

        // Icons
        Texture* m_iconFolder;
        Texture* m_iconTexture;
        Texture* m_iconScene;
        Texture* m_iconAudio;
        Texture* m_iconScript;
        Texture* m_iconFont;
        Texture* m_iconFolderNew = nullptr; 

        float  m_iconSize = 64.0f;
        char   m_filter[128] = {};

        ContentLoader* m_contentLoaderRef;
    };
}
