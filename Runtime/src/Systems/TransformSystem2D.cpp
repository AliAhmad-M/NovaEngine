#include <Runtime/Systems/TransformSystem2D.h>
#include <Core/Components/Parent.h>
#include <Core/Debug.h>
#include <Core/Services.h>
#include <Physics/Components/RigidBody2D.h>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Nova;

static WorldTransform2D Combine(const WorldTransform2D& parent, const Transform2D& local)
{
    const float radians = parent.rotation * (3.14159265358979323846f / 180.0f);
    const float cosR = std::cos(radians);
    const float sinR = std::sin(radians);
    const float offsetX = local.position.x * parent.scale.x;
    const float offsetY = local.position.y * parent.scale.y;

    WorldTransform2D world;
    world.position = parent.position + Vector2(
        offsetX * cosR - offsetY * sinR,
        offsetX * sinR + offsetY * cosR);
    world.rotation = parent.rotation + local.rotation;
    world.scale = Vector2(parent.scale.x * local.scale.x, parent.scale.y * local.scale.y);
    return world;
}

TransformSystem2D::TransformSystem2D()
    : m_ecsRef(Services::Fetch<ECS>())
{
}

void TransformSystem2D::Update(float)
{
    std::unordered_map<EntityID, WorldTransform2D> cache;
    std::unordered_set<EntityID> resolving;
    std::vector<EntityID> resolveStack;

    auto warnOnce = [this](EntityID entity, const char* reason)
    {
        if (m_warnedEntities.insert(entity).second)
            Debug::LogWarn("TransformSystem2D: entity %u %s", static_cast<unsigned int>(entity), reason);
    };

    auto resolve = [&](auto&& self, EntityID entity, unsigned int depth) -> WorldTransform2D
    {
        auto cached = cache.find(entity);
        if (cached != cache.end()) return cached->second;

        Transform2D* local = m_ecsRef->GetComponent<Transform2D>(entity);
        if (!local) return WorldTransform2D{};

        WorldTransform2D world{ local->position, local->rotation, local->scale };
        if (depth >= 64)
        {
            warnOnce(entity, "exceeded the parent transform depth limit of 64");
            cache[entity] = world;
            return world;
        }

        if (m_ecsRef->HasComponent<RigidBody2D>(entity))
        {
            cache[entity] = world;
            return world;
        }

        if (!resolving.insert(entity).second)
        {
            warnOnce(entity, "is part of a parent transform cycle");
            return world;
        }
        resolveStack.push_back(entity);

        if (Parent* parent = m_ecsRef->GetComponent<Parent>(entity))
        {
            if (!m_ecsRef->EntityExists(parent->id) ||
                !m_ecsRef->HasComponent<Transform2D>(parent->id))
            {
                warnOnce(entity, "has a missing or invalid parent; treating it as a root");
            }
            else if (resolving.find(parent->id) != resolving.end())
            {
                auto cycleStart = std::find(resolveStack.begin(), resolveStack.end(), parent->id);
                for (auto it = cycleStart; it != resolveStack.end(); ++it)
                    warnOnce(*it, "is part of a parent transform cycle");
            }
            else
            {
                world = Combine(self(self, parent->id, depth + 1), *local);
            }
        }

        resolveStack.pop_back();
        resolving.erase(entity);
        cache[entity] = world;
        return world;
    };

    for (EntityID entity : entities)
    {
        Transform2D* local = m_ecsRef->GetComponent<Transform2D>(entity);
        WorldTransform2D* world = m_ecsRef->GetComponent<WorldTransform2D>(entity);
        if (!local || !world) continue;
        *world = resolve(resolve, entity, 0);
    }
}

void TransformSystem2D::OnEntityAdded(EntityID) {}
void TransformSystem2D::OnEntityRemoved(EntityID e) { m_warnedEntities.erase(e); }
void TransformSystem2D::OnAllEntitiesRemoved() { m_warnedEntities.clear(); }
