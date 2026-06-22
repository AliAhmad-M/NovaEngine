#pragma once
#include <Runtime/Scene/SceneLoader.h>
#include <Core/Debug.h>
#include <unordered_map>
#include <memory>
#include <string>

namespace Nova 
{
    class SceneManager
    {
    public:
        /**
         * @brief Sets the current active game scene
         * @param path File path to the .json scene
         */
        void ChangeScene(const std::string& path);

    private:
        SceneLoader m_loader;
        std::string m_currentScenePath;
    };
}