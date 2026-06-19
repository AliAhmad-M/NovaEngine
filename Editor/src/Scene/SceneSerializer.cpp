#include <Editor/Scene/SceneSerializer.h>

#include <Core/ECS/ECS.h>
#include <Core/Services.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Physics/Components.h>
#include <World/Components.h>
#include <Graphics/Texture.h>
#include <Runtime/ContentLoader.h>

#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <array>

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

    // Helper to encode possibly-infinite boundary floats as JSON. nlohmann::json
    // cannot represent INFINITY/-INFINITY (they're not valid JSON numbers), so
    // they're stored as sentinel strings and translated back on load.
    nlohmann::json BoundaryValueToJson(float v)
    {
        if (std::isinf(v))
            return v > 0.0f ? "inf" : "-inf";
        return v;
    }

    float BoundaryValueFromJson(const nlohmann::json& j)
    {
        if (j.is_string())
        {
            const std::string& s = j.get_ref<const std::string&>();
            if (s == "inf") return INFINITY;
            if (s == "-inf") return -INFINITY;
            return 0.0f;
        }
        return j.get<float>();
    }

    nlohmann::json SerializeCamera2D(const Camera2D& c)
    {
        nlohmann::json j;
        j["isMainCamera"] = c.isMainCamera;
        j["zoom"] = c.GetZoom();

        std::array<float, 4> boundaries = c.GetBoundaries();
        j["boundaries"] = nlohmann::json{
            {"left",  BoundaryValueToJson(boundaries[0])},
            {"right", BoundaryValueToJson(boundaries[1])},
            {"up",    BoundaryValueToJson(boundaries[2])},
            {"down",  BoundaryValueToJson(boundaries[3])}
        };

        int vpW, vpH;
        c.GetViewPortSize(vpW, vpH);
        j["viewport"] = nlohmann::json{ {"width", vpW}, {"height", vpH} };

        // Offset is derived/clamped state, not authored data, but is stored
        // for convenience so a reloaded camera starts where it was left.
        j["offset"] = Vec2ToJson(c.GetOffset());

        return j;
    }

    nlohmann::json SerializeTilemap2D(const Tilemap2D& tm)
    {
        nlohmann::json j;

        // Tilemap2D's grid data is baked/derived from a source .tmj/.json file
        // and a set of tileset textures. Rather than dumping the (potentially
        // large) baked grids, we record what's needed to reconstruct it:
        // the source map path and the tileset load parameters.
        j["mapPath"] = tm.GetSourceMapPath();
        j["tileWidth"] = tm.GetTileWidth();
        j["tileHeight"] = tm.GetTileHeight();

        nlohmann::json tilesets = nlohmann::json::array();
        for (const TilesetSource2D& src : tm.GetTilesetSources())
        {
            tilesets.push_back(nlohmann::json{
                {"texturePath", src.texturePath},
                {"firstgid", src.firstgid}
                });
        }
        j["tilesets"] = tilesets;

        return j;
    }
}

SceneSerializer::SceneSerializer()
{
    s_contentLoader = Services::Fetch<ContentLoader>();
}

bool SceneSerializer::Save(const EditorScene& scene, const std::filesystem::path& filePath)
{

    ECS* ecs = Services::Fetch<ECS>();
    if (!ecs)
    {
        fprintf(stderr, "[SceneSerializer] EngineContext has no registered ECS\n");
        return false;
    }

    nlohmann::json root;
    root["version"] = 1;
    root["name"] = scene.GetName();

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

        if (ecs->HasComponent<Camera2D>(sceneEntity.id))
        {
            components["Camera2D"] = SerializeCamera2D(*ecs->GetComponent<Camera2D>(sceneEntity.id));
        }

        if (ecs->HasComponent<Sprite2D>(sceneEntity.id))
        {
            components["Sprite2D"] = SerializeSprite2D(*ecs->GetComponent<Sprite2D>(sceneEntity.id));
        }

        if (ecs->HasComponent<RigidBody2D>(sceneEntity.id))
        {
            components["RigidBody2D"] = SerializeRigidBody2D(*ecs->GetComponent<RigidBody2D>(sceneEntity.id));
        }

        if (ecs->HasComponent<Tilemap2D>(sceneEntity.id))
        {
            components["Tilemap2D"] = SerializeTilemap2D(*ecs->GetComponent<Tilemap2D>(sceneEntity.id));
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