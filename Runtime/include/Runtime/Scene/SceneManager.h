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

        /**
         * @brief Get the scene loader reference
         * @return A pointer to the scene loader
         */
        SceneLoader* GetLoader() { return &m_loader; }

    private:
        SceneLoader m_loader;
        std::string m_currentScenePath;
    };
}