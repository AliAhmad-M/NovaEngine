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
    enum class AssetType { Unknown, Texture, Scene, Font, Audio, Script };

    struct AssetEntry
    {
        std::string path;
        std::string filename;
        uintmax_t   fileSize = 0;

        AssetType type = AssetType::Unknown;
        bool isDirectory = false;
    };

    class AssetBrowserPanel
    {
    public:
        explicit AssetBrowserPanel(EditorContext* ctx);

        void Draw();
        void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }
        void SetRoot(const std::filesystem::path& root);

        const std::string& GetLastSelectedPath() const { return m_lastSelectedPath; }

    private:
        void refresh();
        void drawToolbar();
        void drawBreadcrumbs();
        void drawDirectoryTree(const std::filesystem::path& root);
        void drawContentGrid();
        void drawStatusBar();
        void drawModals();

        void importAsset();
        void createNewFolder(const std::string& name);
        void createNewScene(const std::string& name);

        AssetType   classifyExtension(const std::string& ext) const;
        const char* assetTypeLabel(AssetType t, bool isDir) const;
        ImVec4      assetTypeColor(AssetType t, bool isDir) const;
        GLuint      getEntryIcon(const AssetEntry& entry) const;

        EditorContext* m_ctxRef = nullptr;
        ImGuiWindowFlags         m_windowFlags = 0;
        std::filesystem::path    m_rootPath;
        std::filesystem::path    m_currentPath;
        std::vector<AssetEntry>  m_entries;
        std::string              m_lastSelectedPath;
        std::string              m_selectedFile;

        // Modals & Action States
        bool        m_showDeleteModal = false;
        std::string m_itemToDelete;

        bool        m_showRenameModal = false;
        std::string m_itemToRename;
        char        m_renameBuf[256] = {};

        bool        m_showNewFolderModal = false;
        char        m_newFolderBuf[256] = "NewFolder";

        bool        m_showNewSceneModal = false;
        char        m_newSceneBuf[256] = "NewScene";

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
