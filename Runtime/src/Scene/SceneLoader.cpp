#include <Runtime/Scene/SceneLoader.h>
#include <Core/ECS/ECS.h>
#include <Core/Services.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Physics/Components.h>
#include <World/Components.h>
#include <Runtime/ContentLoader.h>

#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <array>

using namespace Nova;

namespace
{
    ContentLoader* s_loader = nullptr;

    float BoundaryValueFromJson(const nlohmann::json& j)
    {
        if (j.is_string())
        {
            const std::string& s = j.get_ref<const std::string&>();
            if (s == "inf")  return  INFINITY;
            if (s == "-inf") return -INFINITY;
            return 0.0f;
        }
        return j.get<float>();
    }

    BehaviorScript DeserializeBehaviorScript(const nlohmann::json& j)
    {
        BehaviorScript bs;
        bs.scriptPath = j.value("scriptPath", "");
        bs.behaviorName = j.value("behaviorName", "");
        return bs;
    }

    Transform2D DeserializeTransform2D(const nlohmann::json& j)
    {
        Transform2D t;
        t.position = { j["position"]["x"].get<float>(), j["position"]["y"].get<float>() };
        t.rotation = j["rotation"].get<float>();
        t.scale = { j["scale"]["x"].get<float>(),    j["scale"]["y"].get<float>() };
        return t;
    }

    Camera2D DeserializeCamera2D(const nlohmann::json& j)
    {
        Camera2D c;
        c.isMain = j["isMainCamera"].get<bool>();
        c.SetZoom(j["zoom"].get<float>());

        std::array<float, 4> boundaries =
        {
            BoundaryValueFromJson(j["boundaries"]["left"]),
            BoundaryValueFromJson(j["boundaries"]["right"]),
            BoundaryValueFromJson(j["boundaries"]["up"]),
            BoundaryValueFromJson(j["boundaries"]["down"])
        };
        c.SetBoundaries(boundaries);

        c.SetViewportSize(j["viewport"]["width"].get<int>(),
            j["viewport"]["height"].get<int>());
        c.SetOffset({ j["offset"]["x"].get<float>(), j["offset"]["y"].get<float>() });
        return c;
    }

    Sprite2D DeserializeSprite2D(const nlohmann::json& j)
    {
        Sprite2D s;

        std::string texPath = j["texturePath"].get<std::string>();
        if (!texPath.empty())
            s.texture = s_loader->LoadTexture(texPath);

        s.hframes = j["hframes"].get<int>();
        s.vframes = j["vframes"].get<int>();
        s.frame = j["frame"].get<int>();
        s.fliph = j["fliph"].get<bool>();
        s.flipv = j["flipv"].get<bool>();
        s.ySortOrigin = j["ySortOrigin"].get<float>();
        s.applyYSort = j["applyYSort"].get<bool>();
        s.zIndex = j["zIndex"].get<int>();

        for (auto& [name, data] : j["animations"].items())
        {
            Sprite2DAnimationData anim;
            anim.frames = data["frames"].get<std::vector<int>>();
            anim.speed = data["speed"];
            s.animations[name] = anim;
        }

        std::string currentAnim = j.value("currentAnimation", "");
        if (!currentAnim.empty() && s.animations.count(currentAnim))
            s.SetAnimation(currentAnim);

        return s;
    }

    RigidBody2D DeserializeRigidBody2D(const nlohmann::json& j)
    {
        Vector2 offset = { j["centerOffset"]["x"].get<float>(),
                           j["centerOffset"]["y"].get<float>() };

        RigidBody2D rb = (j["shape"].get<std::string>() == "Circle")
            ? RigidBody2D(offset, j["radius"].get<float>())
            : RigidBody2D(offset, j["width"].get<float>(), j["height"].get<float>());

        rb.SetStatic(j["isStatic"].get<bool>());
        rb.SetSensor(j["isSensor"].get<bool>());
        rb.SetDensity(j["density"].get<float>());
        rb.SetRestitution(j["restitution"].get<float>());
        rb.SetFriction(j["friction"].get<float>());
        rb.SetLockRotation(j["lockRotation"].get<bool>());
        rb.SetLinearDamping(j["linearDamping"].get<float>());
        rb.SetAngularDamping(j["angularDamping"].get<float>());

        return rb;
    }

    Tilemap2D DeserializeTilemap2D(const nlohmann::json& j)
    {
        Tilemap2D tm(j["tileWidth"].get<int>(), j["tileHeight"].get<int>());

        for (const auto& ts : j["tilesets"])
        {
            std::string texPath = ts["texturePath"].get<std::string>();
            int firstgid = ts["firstgid"].get<int>();
            const Texture* tex = s_loader->LoadTexture(texPath);
            if (tex)
                tm.AddTileset(tex, texPath, firstgid);
            else
                Debug::LogError("Missing tileset texture: %s", texPath.c_str());
        }

        std::string mapPath = j["mapPath"].get<std::string>();
        if (!mapPath.empty())
            tm.LoadTilemap(mapPath);

        return tm;
    }
}

bool SceneLoader::Load(const std::string& path, std::vector<LoadedEntity>* outEntities)
{
    std::ifstream in(path);
    if (!in.is_open())
    {
        Debug::LogError("Failed to open scene: %s", path.c_str());
        return false;
    }

    nlohmann::json root;
    try { root = nlohmann::json::parse(in); }
    catch (const std::exception& e)
    {
        Debug::LogError("Parse error in '%s': %s\n", path.c_str(), e.what());
        return false;
    }

    ECS* ecs = Services::Fetch<ECS>();
    s_loader = Services::Fetch<ContentLoader>();

    ecs->DeleteAllEntities();

    // Create entities from json file
    for (const auto& entityJson : root["entities"])
    {
        EntityID entity = ecs->CreateEntity();
        const auto& comp = entityJson["components"];

        if (comp.contains("BehaviorScript"))
            ecs->AddComponent<BehaviorScript>(entity, DeserializeBehaviorScript(comp["BehaviorScript"]));

        if (comp.contains("Transform2D"))
            ecs->AddComponent<Transform2D>(entity, DeserializeTransform2D(comp["Transform2D"]));

        if (comp.contains("Camera2D"))
            ecs->AddComponent<Camera2D>(entity, DeserializeCamera2D(comp["Camera2D"]));

        if (comp.contains("Sprite2D"))
            ecs->AddComponent<Sprite2D>(entity, DeserializeSprite2D(comp["Sprite2D"]));

        if (comp.contains("RigidBody2D"))
            ecs->AddComponent<RigidBody2D>(entity, DeserializeRigidBody2D(comp["RigidBody2D"]));

        if (comp.contains("Tilemap2D"))
            ecs->AddComponent<Tilemap2D>(entity, DeserializeTilemap2D(comp["Tilemap2D"]));

        if (outEntities)
        {
            EntityID parentId = static_cast<EntityID>(
                entityJson.value("parentId", static_cast<int>(NO_PARENT))
                );
            outEntities->push_back({ entity, entityJson.value("name", "Entity"), parentId });
        }
    }

    Debug::LogInfo("Loaded '%s' (%zu entities)", 
        path.c_str(), root["entities"].size());
    return true;
}