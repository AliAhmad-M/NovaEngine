#include <Editor/Scene/SceneSerializer.h>

#include <Core/ECS/ECS.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Physics/Components.h>
#include <Graphics/Texture.h>
#include <Runtime/ContentLoader.h>

#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

static ContentLoader* s_contentLoader;

namespace
{
    nlohmann::json Vec2ToJson(const Vector2& v)
    {
        return nlohmann::json{ {"x", v.x}, {"y", v.y} };
    }

    nlohmann::json SerializeTransform2D(const Transform2D& t)
    {
        nlohmann::json j;
        j["position"] = Vec2ToJson(t.position);
        j["rotation"] = t.rotation;
        j["scale"] = Vec2ToJson(t.scale);
        return j;
    }

    nlohmann::json SerializeSprite2D(const Sprite2D& s)
    {
        nlohmann::json j;

        // Get texture path
		j["texturePath"] = s_contentLoader->GetTexturePath(s.texture);

        j["hframes"] = s.hframes;
        j["vframes"] = s.vframes;
        j["frame"] = s.frame;
        j["fliph"] = s.fliph;
        j["flipv"] = s.flipv;
        j["zIndex"] = s.GetZIndex();
        j["ySortOrigin"] = s.ySortOrigin;
        j["applyYSort"] = s.applyYSort;
        j["currentAnimation"] = s.GetAnimation();

        nlohmann::json anims = nlohmann::json::object();
        for (const auto& [name, data] : s.animations)
        {
            anims[name] = nlohmann::json
            {
                {"frames", data.frames},
                {"speed",  data.speed}
            };
        }
        j["animations"] = anims;

        return j;
    }

    const char* ShapeToString(Shape2D shape)
    {
        return (shape == Shape2D::CIRCLE) ? "Circle" : "Box";
    }

    nlohmann::json SerializeRigidBody2D(const RigidBody2D& rb)
    {
        nlohmann::json j;
        j["shape"] = ShapeToString(rb.GetShape());
        j["centerOffset"] = Vec2ToJson(rb.centerOffset);

        if (rb.GetShape() == Shape2D::CIRCLE)
        {
            j["radius"] = rb.GetRadius();
        }
        else
        {
            j["width"] = rb.GetFullWidth();
            j["height"] = rb.GetFullHeight();
        }

        j["isStatic"] = rb.IsStatic();
        j["isSensor"] = rb.IsSensor();
        j["density"] = rb.GetDensity();
        j["restitution"] = rb.GetRestitution();
        j["friction"] = rb.GetFriction();
        j["lockRotation"] = rb.LockRotation();
        j["linearDamping"] = rb.GetLinearDamping();
        j["angularDamping"] = rb.GetAngularDamping();
        j["layer"] = static_cast<unsigned>(rb.GetCollisionLayerRaw());
        j["mask"] = static_cast<unsigned>(rb.GetCollisionMaskRaw());

        return j;
    }

    nlohmann::json SerializeCamera(const SceneCamera& cam)
    {
        nlohmann::json j;
        j["boundary"] = nlohmann::json{
            {"left",  cam.left},
            {"right", cam.right},
            {"up",    cam.up},
            {"down",  cam.down}
        };
        j["offset"] = Vec2ToJson(cam.offset);
        j["zoom"] = cam.zoom;
        return j;
    }
}

SceneSerializer::SceneSerializer(EngineContext* engineContext)
    : m_engineContext(engineContext)
{
	s_contentLoader = m_engineContext->Get<ContentLoader>();
}

bool SceneSerializer::Save(const EditorScene& scene, const std::filesystem::path& filePath)
{
    if (!m_engineContext)
    {
        fprintf(stderr, "[SceneSerializer] No EngineContext bound\n");
        return false;
    }

    ECS* ecs = m_engineContext->Get<ECS>();
    if (!ecs)
    {
        fprintf(stderr, "[SceneSerializer] EngineContext has no registered ECS\n");
        return false;
    }

    nlohmann::json root;
    root["version"] = 1;
    root["name"] = scene.GetName();
    root["camera"] = SerializeCamera(scene.GetCamera());

    nlohmann::json entitiesArray = nlohmann::json::array();

    for (const SceneEntity& sceneEntity : scene.GetEntities())
    {
        nlohmann::json entityJson;
        entityJson["id"] = sceneEntity.id;
        entityJson["name"] = sceneEntity.name;

        nlohmann::json components = nlohmann::json::object();

        // Add one block per known component type
        if (ecs->HasComponent<Transform2D>(sceneEntity.id))
        {
            components["Transform2D"] = SerializeTransform2D(*ecs->GetComponent<Transform2D>(sceneEntity.id));
        }

        if (ecs->HasComponent<Sprite2D>(sceneEntity.id))
        {
            components["Sprite2D"] = SerializeSprite2D(*ecs->GetComponent<Sprite2D>(sceneEntity.id));
        }

        if (ecs->HasComponent<RigidBody2D>(sceneEntity.id))
        {
            components["RigidBody2D"] = SerializeRigidBody2D(*ecs->GetComponent<RigidBody2D>(sceneEntity.id));
        }

        entityJson["components"] = components;
        entitiesArray.push_back(std::move(entityJson));
    }

    root["entities"] = entitiesArray;

    // Ensure parent directories exist
    std::error_code ec;
    std::filesystem::create_directories(filePath.parent_path(), ec);
    if (ec)
    {
        fprintf(stderr, "[SceneSerializer] Failed to create directories for '%s': %s\n",
            filePath.string().c_str(), ec.message().c_str());
        return false;
    }

    std::ofstream out(filePath);
    if (!out.is_open())
    {
        fprintf(stderr, "[SceneSerializer] Failed to open '%s' for writing\n", filePath.string().c_str());
        return false;
    }

    out << root.dump(4);
    return true;
}