#pragma once

#include <string>
#include <vector>
#include <Core/Utilities/Vector2.h>
#include <Core/ECS/ECS.h>

namespace Nova::Editor 
{
    /**
     * @brief A live entity tracked by the editor, paired with a display name.
     */
    struct SceneEntity
    {
        EntityID    id = 0;
        std::string name;
    };

    /**
     * @brief Global camera for a scene: clamp boundaries, pan offset, and zoom.
     */
    struct SceneCamera
    {
        float left = 0.0f;
        float right = 0.0f;
        float up = 0.0f;
        float down = 0.0f;

        Vector2 offset = Vector2::ZERO;
        float   zoom = 1.0f;
    };

    class Scene
    {
    public:
        Scene() = default;
        explicit Scene(std::string name) : m_name(std::move(name)) {}

        const std::string& GetName() const { return m_name; }
        void SetName(const std::string& name) { m_name = name; }

        const std::string& GetFilePath() const { return m_filePath; }
        void SetFilePath(const std::string& path) { m_filePath = path; }

        SceneCamera& GetCamera() { return m_camera; }
        const SceneCamera& GetCamera() const { return m_camera; }

        std::vector<SceneEntity>& GetEntities() { return m_entities; }
        const std::vector<SceneEntity>& GetEntities() const { return m_entities; }

        /**
         * @brief Track a newly created entity under this scene
         * @param id    The EntityID of the new entity
         * @param name  Display name for the hierarchy panel
         */
        void AddEntity(EntityID id, const std::string& name)
        {
            m_entities.push_back({ id, name });
        }

        /**
         * @brief Stop tracking an entity (does NOT destroy it in the ECS)
         */
        void RemoveEntity(EntityID id)
        {
            for (auto it = m_entities.begin(); it != m_entities.end(); ++it)
            {
                if (it->id == id)
                {
                    m_entities.erase(it);
                    return;
                }
            }
        }

        SceneEntity* FindEntity(EntityID id)
        {
            for (auto& e : m_entities)
                if (e.id == id) return &e;
            return nullptr;
        }

    private:
        std::string m_name = "Untitled";
        std::string m_filePath;
        SceneCamera m_camera;
        std::vector<SceneEntity> m_entities;
    };
}