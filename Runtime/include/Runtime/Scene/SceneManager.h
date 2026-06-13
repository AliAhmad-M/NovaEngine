#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include "Scene.h"

class SceneManager
{
public:
    /**
     * @brief Instantiates a new scene type and adds it to the manager
     * @tparam T The class type of the scene to create (must derive from IScene)
     * @param name The unique identifier name for the scene
     */
    template <typename T>
    void CreateScene(const std::string& name, EngineContext* context)
    {
        if (m_scenes.find(name) != m_scenes.end())
        {
            fprintf(stderr, "[WARN]: Scene '%s' has already been added\n", name.c_str());
            return;
        }

        std::shared_ptr<Scene> scene = std::make_shared<T>(context);
        m_scenes[name] = std::move(scene);
    }

    /**
     * @brief Updates the currently active scene
     * @param dt The delta time passed since the last frame
     */
    void UpdateScene(float dt);

    /**
     * @brief Removes a scene from management and unloads its assets
     * @param name The unique identifier name of the scene to delete
     */
    void DeleteScene(const std::string& name);

    /**
     * @brief Sets the active scene to be updated and rendered next frame
     * @param name The unique identifier name of the scene to load
     */
    void ChangeScene(const std::string& name);

private:
    // Map from scene name to scene itself
    std::unordered_map<std::string, std::shared_ptr<Scene>> m_scenes{};

    // The current scene that is loaded in
    Scene* m_currentScene = nullptr;
};