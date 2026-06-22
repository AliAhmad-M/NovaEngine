#pragma once

#include <Core/ECS/ECS.h>
#include <string>
#include <vector>

namespace Nova
{
    struct LoadedEntity
    {
        EntityID    id;
        std::string name;
    };

    class SceneLoader
    {
    public:
        /**
         * @brief Loads a new scene from file, clears previous one
         * @param path File path to the .json scene
         * @return True on successful scene load, false otherwise
         */
        bool Load(const std::string& path,
            std::vector<LoadedEntity>* outEntities = nullptr);
    };
}