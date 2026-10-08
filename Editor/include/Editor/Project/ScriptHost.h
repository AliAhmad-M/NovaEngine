#pragma once
#include <Editor/Project/ProjectManager.h>
#include <functional>
#include <future>
#include <string>
#include <vector>

namespace Nova::Editor
{
    class ScriptHost
    {
    public:
        ~ScriptHost() { Unload(); }

        void BuildAsync(const ProjectInfo& p);
        bool IsBuilding() const { return m_build.valid(); }
        bool PollBuild(std::string& log, bool& ok);

        bool Load(const ProjectInfo& p);
        void Unload();
        bool IsLoaded() const { return m_handle != nullptr; }

        // Destroy every live behavior here
        std::function<void()> onBeforeUnload;

    private:
        void* m_handle = nullptr;
        std::vector<std::string> m_registered;
        std::future<int> m_build;
        std::string m_logPath;
        int m_loadCounter = 0;
    };
}