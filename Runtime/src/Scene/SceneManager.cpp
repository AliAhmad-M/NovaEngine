#include <Runtime/Scene/SceneManager.h>

using namespace Nova;

void SceneManager::ChangeScene(const std::string& path) 
{
    SceneLoader loader;
    if (!loader.Load(path))
    {
        Debug::LogError("Failed to load scene: %s", path.c_str());
        return;
    }
    m_currentScenePath = path;
}