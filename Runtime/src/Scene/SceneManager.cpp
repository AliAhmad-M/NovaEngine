#include <Runtime/Scene/SceneManager.h>

void SceneManager::UpdateScene(float dt) 
{
	if (m_currentScene) 
	{
		m_currentScene->Update(dt);
	} 
}

void SceneManager::DeleteScene(const std::string& name) 
{
	if (m_scenes.find(name) == m_scenes.end()) 
	{
		fprintf(stderr, "[WARN], Cannot delete scene '%s' as it does not exist\n", name.c_str());
		return;
	}

	// If the scene to delete is current scene
	if (m_currentScene == m_scenes[name].get())
	{
		m_currentScene = nullptr; // Clear current scene
	}
	m_scenes.erase(name);
	printf("[INFO]: Successfully deleted scene: %s\n", name.c_str());
}

void SceneManager::ChangeScene(const std::string& name) 
{
	if (m_scenes.find(name) == m_scenes.end()) 
	{
		fprintf(stderr, "[WARN], Cannot change to scene '%s' as it does not exist\n", name.c_str());
		return;
	}

	Scene* nextScene = m_scenes[name].get();
	if (m_currentScene) 
	{
		m_currentScene->Leave();
	}
	m_currentScene = nextScene;
	m_currentScene->Ready();
	printf("[INFO]: Successfully changed scene to: %s\n", name.c_str());
}