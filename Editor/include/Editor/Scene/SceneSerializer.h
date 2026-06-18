#pragma once

#include <string>
#include <filesystem>
#include <Core/EngineContext.h>
#include <Editor/Scene/EditorScene.h>

/**
 * @brief Saves a Scene to a json file on the disk
 */
class SceneSerializer
{
public:
    /**
     * @brief Construct a serializer bound to the engine's service locator.
	 * @param engineContext Pointer to the engine context for accessing ECS
     */
    explicit SceneSerializer(EngineContext* engineContext);

    /**
     * @brief Save a scene to disk as JSON
     * @param scene     Scene to serialize
     * @param filePath  Absolute or relative path, inside the project
     * @return true on success, false on failure
     */
    bool Save(const EditorScene& scene, const std::filesystem::path& filePath);

private:
    EngineContext* m_engineContext = nullptr;
};