#pragma once

#include <Editor/Scene/Scene.h>
#include <string>
#include <filesystem>

namespace Nova::Editor 
{

    /**
     * @brief Saves a Scene to a json file on the disk
     */
    class SceneSerializer
    {
    public:
        /**
         * @brief Construct a serializer bound to the engine's service locator.
         */
        explicit SceneSerializer();

        /**
         * @brief Save a scene to disk as JSON
         * @param scene     Scene to serialize
         * @param filePath  Absolute or relative path, inside the project
         * @return true on success, false on failure
         */
        bool Save(const Scene& scene, const std::filesystem::path& filePath);
    };
}
