#pragma once

#include <string>
#include <vector>
#include <Core/Utilities/Vector2.h>
#include <Core/ECS/ECS.h>

namespace Nova::Editor
{
    static constexpr EntityID NO_PARENT = static_cast<EntityID>(-1);

    struct SceneEntity
    {
        EntityID    id = 0;
        std::string name;
        EntityID    parentId = NO_PARENT;  // add this
    };

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

        void AddEntity(EntityID id, const std::string& name,
            EntityID parentId = NO_PARENT)
        {
            m_entities.push_back({ id, name, parentId });
        }

        void RemoveEntity(EntityID id)
        {
            // Unparent children of removed entity
            for (auto& e : m_entities)
                if (e.parentId == id)
                    e.parentId = NO_PARENT;

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

        // Set parent of an entity
        void SetParent(EntityID childId, EntityID parentId)
        {
            SceneEntity* child = FindEntity(childId);
            if (child) child->parentId = parentId;
        }

        // Get direct children of an entity
        std::vector<EntityID> GetChildren(EntityID parentId) const
        {
            std::vector<EntityID> children;
            for (const auto& e : m_entities)
                if (e.parentId == parentId)
                    children.push_back(e.id);
            return children;
        }

    private:
        std::string              m_name = "Untitled";
        std::string              m_filePath;
        SceneCamera              m_camera;
        std::vector<SceneEntity> m_entities;
    };
}