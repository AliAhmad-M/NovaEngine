#pragma once

#include <string>

namespace Nova
{
    class SceneLoader
    {
    public:
        /**
         * @brief Loads a new scene from file, clears previous one
         * @param path File path to the .json scene
         * @return True on successful scene load, false otherwise
         */
        bool Load(const std::string& path);
    };
}