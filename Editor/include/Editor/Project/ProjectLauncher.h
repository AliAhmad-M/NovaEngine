#pragma once
#include <Editor/Project/ProjectManager.h>
#include <functional>
#include <string>

namespace Nova::Editor
{
    class ProjectLauncher
    {
    public:
        ProjectLauncher();
        void Draw(ProjectManager& pm, const std::function<void(const std::string&)>& onOpen);
        void SetError(const std::string& e) { m_error = e; }

    private:
        char m_name[64] = "My Game";
        char m_location[512] = {};
        char m_openPath[512] = {};
        std::string m_error;
        std::string m_pickRoot;
    };
}