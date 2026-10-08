#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Nova::Editor
{
    struct ProjectEntry
    {
        std::string  name;
        std::string  path;          // project root
        std::int64_t lastOpened = 0;
    };

    struct ProjectInfo
    {
        std::string           name;
        std::filesystem::path root;
        std::filesystem::path assets;   // root/assets
        std::filesystem::path scenes;   // root/scenes
        std::filesystem::path scripts;  // root/scripts
        std::filesystem::path build;    // root/build
    };

    class ProjectManager
    {
    public:
        static constexpr const char* kDescriptor = "project.nova";

        void LoadRecent();
        void SaveRecent() const;
        const std::vector<ProjectEntry>& Recent() const { return m_recent; }
        void Forget(const std::string& path);

        // Creates the folder
        bool Create(const std::string& parentDir, const std::string& name,
            std::string& outRoot, std::string& err);
        bool Open(const std::string& rootDir, std::string& err);
        void Close() { m_hasProject = false; }

        bool HasProject() const { return m_hasProject; }
        const ProjectInfo& Current() const { return m_current; }

        static std::filesystem::path SdkDir();
        static std::filesystem::path ConfigDir();

    private:
        void touchRecent(const ProjectInfo& p);

        std::vector<ProjectEntry> m_recent;
        ProjectInfo m_current;
        bool m_hasProject = false;
    };
}