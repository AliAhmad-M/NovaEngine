#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Panels/InspectorPanel.h>
#include <Core/ECS/ECS.h>
#include <Core/Behavior/BehaviorRegistry.h>
#include <Core/Services.h>
#include <Graphics/Texture.h>
#include <Editor/FilePicker.h>
#include <imgui.h>
#include <cstring>
#include <cmath>
#include <array>
#include <sstream>
#include <vector>

using namespace Nova::Editor;

InspectorPanel::InspectorPanel(Context* ctx) : m_ctx(ctx)
{
    m_ecs = Services::Fetch<ECS>();
    m_contentLoader = Services::Fetch<ContentLoader>();

    m_iconTransform  = m_contentLoader->LoadTexture("assets/icons/move.png");
    m_iconSprite     = m_contentLoader->LoadTexture("assets/icons/image.png");
    m_iconCamera     = m_contentLoader->LoadTexture("assets/icons/icon_camera.png");
    m_iconRigidbody  = m_contentLoader->LoadTexture("assets/icons/atom.png");
    m_iconTilemap    = m_contentLoader->LoadTexture("assets/icons/grid.png");
    m_iconScript     = m_contentLoader->LoadTexture("assets/icons/script.png");
}

static void FieldVec2(const char* label, float* v, float speed = 0.1f)
{
    ImGui::PushID(label);

    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            v[0] += ImGui::GetIO().MouseDelta.x * speed;
            v[1] += ImGui::GetIO().MouseDelta.x * speed;
        }
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputFloat2("##v", v, "%.3f");

    ImGui::PopID();
}


static void FieldFloat(const char* label, float* v, float speed = 0.01f,
    float vmin = 0.0f, float vmax = 0.0f)
{
    ImGui::PushID(label);

    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            *v += ImGui::GetIO().MouseDelta.x * speed;
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputFloat("##v", v, 0.0f, 0.0f, "%.3f");  // 0,0 hides +- buttons

    ImGui::PopID();
}

static void FieldInt(const char* label, int* v, float speed = 1.0f,
    int vmin = 0, int vmax = 0)
{
    ImGui::PushID(label);

    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            *v += (int)(ImGui::GetIO().MouseDelta.x * speed);
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputInt("##v", v, 0, 0);  // 0,0 hides +- buttons

    ImGui::PopID();
}

static void FieldBool(const char* label, bool* v)
{
    ImGui::PushID(label);
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Checkbox("##v", v);
    ImGui::PopID();
}

static void FieldString(const char* label, char* buf, size_t bufSize)
{
    ImGui::PushID(label);
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##v", buf, bufSize);
    ImGui::PopID();
}

static void FieldReadOnly(const char* label, const char* value)
{
    ImGui::PushID(label);
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(value);
    ImGui::PopID();
}

static bool BeginComponent(const char* name, bool* removeRequested = nullptr,
    ImTextureID icon = 0)
{
    ImGui::PushID(name);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_AllowOverlap |
        ImGuiTreeNodeFlags_FramePadding;

    // Draw the arrow/frame only (empty label) so we control icon + text placement ourselves
    bool open = ImGui::TreeNodeEx("##header", flags);

    float lineHeight = ImGui::GetFrameHeight();
    float textHeight = ImGui::GetTextLineHeight();

    // Icon comes right after the arrow
    ImGui::SameLine();
    if (icon)
    {
        float iconSize = 16.0f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (lineHeight - iconSize) * 0.5f);
        ImGui::Image(icon, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1));
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (lineHeight - iconSize) * 0.5f);
    }

    // Label after the icon, vertically centered in the row
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (lineHeight - textHeight) * 0.5f);
    ImGui::TextUnformatted(name);
    

    // Three dots button on the right side of header
    float btnW = ImGui::GetFrameHeight();
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - btnW + ImGui::GetCursorPosX()
        - ImGui::GetStyle().ItemSpacing.x);

    if (ImGui::Button("...##opts", ImVec2(btnW, btnW)))
        ImGui::OpenPopup("##ComponentCtx");

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##ComponentCtx");

    if (ImGui::BeginPopup("##ComponentCtx"))
    {
        ImGui::TextDisabled("%s", name);
        ImGui::Separator();
        if (ImGui::MenuItem("Reset")) {}
        ImGui::Separator();
        if (ImGui::MenuItem("Move Up")) {}
        if (ImGui::MenuItem("Move Down")) {}
        ImGui::Separator();
        if (removeRequested && ImGui::MenuItem("Remove Component"))
            *removeRequested = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Copy Component")) {}
        if (ImGui::MenuItem("Paste Component Values")) {}
        ImGui::EndPopup();
    }

    if (!open)
        ImGui::PopID();

    return open;
}

static void EndComponent()
{
    ImGui::TreePop();
    ImGui::PopID();
}

static void BeginPropertyTable()
{
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 3));
    ImGui::BeginTable("##props", 2,
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV);
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
}

static void EndPropertyTable()
{
    ImGui::EndTable();
    ImGui::PopStyleVar();
}

static void NextRow() { ImGui::TableNextRow(); }

void InspectorPanel::Draw()
{
    ImGui::Begin("Inspector");

    if (m_ctx->selectedEntity == -1)
    {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    EntityID entity = static_cast<EntityID>(m_ctx->selectedEntity);

    char nameLabel[64];
    std::snprintf(nameLabel, sizeof(nameLabel), "Entity %d", m_ctx->selectedEntity);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::Text("%s", nameLabel);
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::Spacing();

    // draw each component section if the entity has that component
    if (Transform2D* t = m_ecs->GetComponent<Transform2D>(entity))
        drawTransform2D(*t);

    if (Camera2D* c = m_ecs->GetComponent<Camera2D>(entity))
        drawCamera2D(*c);

    if (Sprite2D* s = m_ecs->GetComponent<Sprite2D>(entity))
        drawSprite2D(*s);

    if (RigidBody2D* rb = m_ecs->GetComponent<RigidBody2D>(entity))
        drawRigidBody2D(*rb);

    if (Tilemap2D* tm = m_ecs->GetComponent<Tilemap2D>(entity))
        drawTilemap2D(*tm);

    if (BehaviorScript* bs = m_ecs->GetComponent<BehaviorScript>(entity))
        drawBehaviorScript(*bs);

    ImGui::Spacing();
    drawAddComponentMenu();

    if (m_pendingPicker.has_value())
    {
        ImGui::OpenPopup(m_pendingPicker->popupId);
        m_pickerOpen = true;
    }

    if (m_pickerOpen && m_pendingPicker.has_value())
    {
        auto& req = *m_pendingPicker;
        std::string path = "../Test/project";
        if (!DrawFilePicker(req.popupId, path, req.filterExt, req.onConfirm))
        {
            m_pendingPicker.reset();
            m_pickerOpen = false;
        }
    }

    ImGui::End();
}

void InspectorPanel::drawAddComponentMenu()
{
    EntityID entity = static_cast<EntityID>(m_ctx->selectedEntity);

    float btnW = 200.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - btnW) * 0.5f);
    if (ImGui::Button("Add Component", ImVec2(btnW, 0)))
        ImGui::OpenPopup("AddComponentPopup");

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        ImGui::TextDisabled("Components");
        ImGui::Separator();

        if (!m_ecs->HasComponent<Transform2D>(entity) && ImGui::MenuItem("Transform2D"))
            m_ecs->AddComponent<Transform2D>(entity, Transform2D());

        if (!m_ecs->HasComponent<Camera2D>(entity) && ImGui::MenuItem("Camera2D"))
            m_ecs->AddComponent<Camera2D>(entity, Camera2D());

        if (!m_ecs->HasComponent<Sprite2D>(entity) && ImGui::MenuItem("Sprite2D"))
            m_ecs->AddComponent<Sprite2D>(entity, Sprite2D());

        if (!m_ecs->HasComponent<RigidBody2D>(entity) && ImGui::MenuItem("RigidBody2D (Circle)"))
            m_ecs->AddComponent<RigidBody2D>(entity, RigidBody2D(Vector2::ZERO, PHYSICS_PPM));

        if (!m_ecs->HasComponent<RigidBody2D>(entity) && ImGui::MenuItem("RigidBody2D (Box)"))
            m_ecs->AddComponent<RigidBody2D>(entity, RigidBody2D(Vector2::ZERO, PHYSICS_PPM, PHYSICS_PPM));

        if (!m_ecs->HasComponent<Tilemap2D>(entity) && ImGui::MenuItem("Tilemap2D"))
            m_ecs->AddComponent<Tilemap2D>(entity, Tilemap2D());

        if (!m_ecs->HasComponent<BehaviorScript>(entity) && ImGui::MenuItem("Script"))
            m_ecs->AddComponent<BehaviorScript>(entity, BehaviorScript());

        ImGui::EndPopup();
    }
}

void InspectorPanel::drawTransform2D(Transform2D& t)
{
    bool removeRequested = false;
    ImTextureID iconHandle = m_iconTransform ? (ImTextureID)(intptr_t)m_iconTransform->GetHandle() : 0;
    if (!BeginComponent("Transform2D", &removeRequested, iconHandle)) return;

    BeginPropertyTable();
    NextRow(); FieldVec2("Position", &t.position.x);
    NextRow(); FieldFloat("Rotation", &t.rotation, 0.5f);
    NextRow(); FieldVec2("Scale", &t.scale.x);
    EndPropertyTable();

    EndComponent();

    if (removeRequested)
    {
        ECS* m_ecs = Services::Fetch<ECS>();
        m_ecs->RemoveComponent<Transform2D>(static_cast<EntityID>(m_ctx->selectedEntity));
    }
}

void InspectorPanel::drawCamera2D(Camera2D& c)
{
    bool removeRequested = false;
    ImTextureID iconHandle = m_iconCamera ? (ImTextureID)(intptr_t)m_iconCamera->GetHandle() : 0;
    if (!BeginComponent("Camera2D", &removeRequested, iconHandle)) return;

    BeginPropertyTable();

    NextRow();
    bool isMain = c.isMainCamera;
    FieldBool("Main Camera", &isMain);
    if (isMain != c.isMainCamera) c.isMainCamera = isMain;

    NextRow();
    float zoom = c.GetZoom();
    FieldFloat("Zoom", &zoom, 0.01f, 0.01f, 0.0f);
    if (zoom != c.GetZoom())
        c.SetZoom(zoom);

    // boundaries: left, right, up, down
    std::array<float, 4> boundaries = c.GetBoundaries();
    bool boundsChanged = false;

    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Bound Left");
    ImGui::TableSetColumnIndex(1);
    {
        ImGui::PushID("BoundLeft");
        bool unbounded = std::isinf(boundaries[0]);
        float v = unbounded ? 0.0f : boundaries[0];
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
        if (!unbounded && ImGui::DragFloat("##v", &v, 1.0f)) { boundaries[0] = v; boundsChanged = true; }
        if (unbounded) { ImGui::BeginDisabled(); ImGui::DragFloat("##v", &v, 1.0f); ImGui::EndDisabled(); }
        ImGui::SameLine();
        if (ImGui::Checkbox("∞##inf", &unbounded)) { boundaries[0] = unbounded ? -INFINITY : 0.0f; boundsChanged = true; }
        ImGui::PopID();
    }

    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Bound Right");
    ImGui::TableSetColumnIndex(1);
    {
        ImGui::PushID("BoundRight");
        bool unbounded = std::isinf(boundaries[1]);
        float v = unbounded ? 0.0f : boundaries[1];
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
        if (!unbounded && ImGui::DragFloat("##v", &v, 1.0f)) { boundaries[1] = v; boundsChanged = true; }
        if (unbounded) { ImGui::BeginDisabled(); ImGui::DragFloat("##v", &v, 1.0f); ImGui::EndDisabled(); }
        ImGui::SameLine();
        if (ImGui::Checkbox("∞##inf", &unbounded)) { boundaries[1] = unbounded ? INFINITY : 0.0f; boundsChanged = true; }
        ImGui::PopID();
    }

    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Bound Up");
    ImGui::TableSetColumnIndex(1);
    {
        ImGui::PushID("BoundUp");
        bool unbounded = std::isinf(boundaries[2]);
        float v = unbounded ? 0.0f : boundaries[2];
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
        if (!unbounded && ImGui::DragFloat("##v", &v, 1.0f)) { boundaries[2] = v; boundsChanged = true; }
        if (unbounded) { ImGui::BeginDisabled(); ImGui::DragFloat("##v", &v, 1.0f); ImGui::EndDisabled(); }
        ImGui::SameLine();
        if (ImGui::Checkbox("∞##inf", &unbounded)) { boundaries[2] = unbounded ? -INFINITY : 0.0f; boundsChanged = true; }
        ImGui::PopID();
    }

    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Bound Down");
    ImGui::TableSetColumnIndex(1);
    {
        ImGui::PushID("BoundDown");
        bool unbounded = std::isinf(boundaries[3]);
        float v = unbounded ? 0.0f : boundaries[3];
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
        if (!unbounded && ImGui::DragFloat("##v", &v, 1.0f)) { boundaries[3] = v; boundsChanged = true; }
        if (unbounded) { ImGui::BeginDisabled(); ImGui::DragFloat("##v", &v, 1.0f); ImGui::EndDisabled(); }
        ImGui::SameLine();
        if (ImGui::Checkbox("∞##inf", &unbounded)) { boundaries[3] = unbounded ? INFINITY : 0.0f; boundsChanged = true; }
        ImGui::PopID();
    }

    if (boundsChanged)
        c.SetBoundaries(boundaries);

    int vpW, vpH;
    c.GetViewPortSize(vpW, vpH);
    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Viewport");
    ImGui::TableSetColumnIndex(1);
    int viewport[2] = { vpW, vpH };
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::DragInt2("##viewport", viewport, 1.0f, 1, 0))
        c.SetViewportSize(viewport[0], viewport[1]);

    Vector2 offset = c.GetOffset();
    float offsetArr[2] = { offset.x, offset.y };
    NextRow(); FieldVec2("Offset", offsetArr, 1.0f);
    if (offsetArr[0] != offset.x || offsetArr[1] != offset.y)
        c.SetOffset({ offsetArr[0], offsetArr[1] });

    EndPropertyTable();
    EndComponent();

    if (removeRequested)
    {
        m_ecs->RemoveComponent<Camera2D>(static_cast<EntityID>(m_ctx->selectedEntity));
    }
}

void InspectorPanel::drawTexturePathField(Sprite2D& s)
{
    static char pathBuf[256];
    std::string currentPath = m_contentLoader->GetTexturePath(s.texture);
    std::snprintf(pathBuf, sizeof(pathBuf), "%s", currentPath.c_str());

    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Texture");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-28.0f);

    if (ImGui::InputText("##texPath", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        Texture* loaded = m_contentLoader->LoadTexture(pathBuf);
        if (loaded) s.texture = loaded;
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("..."))
    {
        m_pendingPicker = {
            "##pickerTex", ".png",
            [&s](const std::string& path)
            {
                Texture* loaded = Services::Fetch<ContentLoader>()->LoadTexture(path);
                if (loaded) s.texture = loaded;
            }
        };
    }
}

void InspectorPanel::drawSprite2D(Sprite2D& s)
{
    bool removeRequested = false;
    ImTextureID iconHandle = m_iconSprite ? (ImTextureID)(intptr_t)m_iconSprite->GetHandle() : 0;
    if (!BeginComponent("Sprite2D", &removeRequested, iconHandle)) return;

    BeginPropertyTable();

    NextRow(); drawTexturePathField(s);
    NextRow(); FieldInt("H Frames", &s.hframes, 1, 1, 0);
    NextRow(); FieldInt("V Frames", &s.vframes, 1, 1, 0);
    NextRow(); FieldInt("Frame", &s.frame, 1, 0, 0);
    NextRow(); FieldBool("Flip H", &s.fliph);
    NextRow(); FieldBool("Flip V", &s.flipv);
    NextRow(); FieldFloat("Y Sort Origin", &s.ySortOrigin, 0.5f);
    NextRow(); FieldBool("Apply Y Sort", &s.applyYSort);

    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Z Index");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    int zIndex = s.zIndex;
    if (ImGui::DragInt("##zindex", &zIndex))
        s.zIndex = zIndex;

    EndPropertyTable();
    EndComponent();

    if (removeRequested)
    {
        m_ecs->RemoveComponent<Sprite2D>(static_cast<EntityID>(m_ctx->selectedEntity));
    }
}

void InspectorPanel::drawRigidBody2D(RigidBody2D& rb)
{
    bool removeRequested = false;
    ImTextureID iconHandle = m_iconRigidbody ? (ImTextureID)(intptr_t)m_iconRigidbody->GetHandle() : 0;
    if (!BeginComponent("RigidBody2D", &removeRequested, iconHandle)) return;

    BeginPropertyTable();

    NextRow(); FieldVec2("Center Offset", &rb.centerOffset.x);

    if (rb.GetShape() == Shape2D::CIRCLE)
    {
        NextRow();
        ImGui::PushID("Radius");
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Radius");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
        float radius = rb.GetRadius();
        if (ImGui::DragFloat("##radius", &radius, 0.5f, 0.001f))
            rb.SetRadius(radius);
        ImGui::PopID();
    }
    else
    {
        NextRow();
        ImGui::PushID("Width");
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Width");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
        float width = rb.GetFullWidth();
        if (ImGui::DragFloat("##width", &width, 0.5f, 0.001f))
            rb.SetDimensions(width, rb.GetFullHeight());
        ImGui::PopID();

        NextRow();
        ImGui::PushID("Height");
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Height");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
        float height = rb.GetFullHeight();
        if (ImGui::DragFloat("##height", &height, 0.5f, 0.001f))
            rb.SetDimensions(rb.GetFullWidth(), height);
        ImGui::PopID();
    }

    NextRow();
    bool isStatic = rb.IsStatic();
    FieldBool("Static", &isStatic);
    if (isStatic != rb.IsStatic()) rb.SetStatic(isStatic);

    NextRow();
    bool isSensor = rb.IsSensor();
    FieldBool("Sensor", &isSensor);
    if (isSensor != rb.IsSensor()) rb.SetSensor(isSensor);

    NextRow();
    float density = rb.GetDensity();
    FieldFloat("Density", &density, 0.01f, 0.0f, 0.0f);
    if (density != rb.GetDensity()) rb.SetDensity(density);

    NextRow();
    float restitution = rb.GetRestitution();
    FieldFloat("Restitution", &restitution, 0.01f, 0.0f, 1.0f);
    if (restitution != rb.GetRestitution()) rb.SetRestitution(restitution);

    NextRow();
    float friction = rb.GetFriction();
    FieldFloat("Friction", &friction, 0.01f, 0.0f, 1.0f);
    if (friction != rb.GetFriction()) rb.SetFriction(friction);

    NextRow();
    bool lockRotation = rb.LockRotation();
    FieldBool("Lock Rotation", &lockRotation);
    if (lockRotation != rb.LockRotation()) rb.SetLockRotation(lockRotation);

    NextRow();
    float linearDamping = rb.GetLinearDamping();
    FieldFloat("Linear Damping", &linearDamping, 0.01f, 0.0f, 0.0f);
    if (linearDamping != rb.GetLinearDamping()) rb.SetLinearDamping(linearDamping);

    NextRow();
    float angularDamping = rb.GetAngularDamping();
    FieldFloat("Angular Damping", &angularDamping, 0.01f, 0.0f, 0.0f);
    if (angularDamping != rb.GetAngularDamping()) rb.SetAngularDamping(angularDamping);

    EndPropertyTable();
    EndComponent();

    if (removeRequested)
    {
        m_ecs->RemoveComponent<RigidBody2D>(static_cast<EntityID>(m_ctx->selectedEntity));
    }
}

static bool s_rebuildPending = false;

void InspectorPanel::drawTilemap2D(Tilemap2D& tm)
{
    bool removeRequested = false;
    ImTextureID iconHandle = m_iconTilemap ? (ImTextureID)(intptr_t)m_iconTilemap->GetHandle() : 0;
    if (!BeginComponent("Tilemap2D", &removeRequested, iconHandle)) return;

    static char s_mapPathBuf[512] = {};
    static char s_tileGidsBuf[256] = {};
    static int  s_tileW = 16;
    static int  s_tileH = 16;
    static int  s_collisionLayer = 0;

    static std::vector<TilesetSource2D> s_tilesets;

    static char s_pendingPath[512] = {};
    static int  s_pendingFirstgid = -1;

    static std::string s_seededFor;
    const std::string& srcPath = tm.GetSourceMapPath();
    if (s_seededFor != srcPath && !srcPath.empty())
    {
        std::strncpy(s_mapPathBuf, srcPath.c_str(), sizeof(s_mapPathBuf) - 1);
        s_tileW = tm.GetTileWidth();
        s_tileH = tm.GetTileHeight();
        s_tilesets = std::vector<TilesetSource2D>(
            tm.GetTilesetSources().begin(), tm.GetTilesetSources().end());
        s_seededFor = srcPath;
    }

    auto tryRebuild = [&]()
        {
            if (s_tilesets.empty() || s_mapPathBuf[0] == '\0') return;

            ContentLoader* loader = Services::Fetch<ContentLoader>();
            if (!loader) return;

            Tilemap2D fresh(s_tileW, s_tileH);

            for (const auto& src : s_tilesets)
            {
                const Texture* tex = loader->LoadTexture(src.texturePath);
                if (!tex)
                {
                    Debug::LogError("[Tilemap] Failed to load tileset: %s", src.texturePath.c_str());
                    return;
                }
                fresh.AddTileset(tex, src.texturePath, src.firstgid);
            }

            fresh.LoadTilemap(std::string(s_mapPathBuf));

            if (s_tileGidsBuf[0] != '\0')
            {
                std::vector<int> tileIds;
                std::istringstream ss(s_tileGidsBuf);
                std::string token;
                while (std::getline(ss, token, ','))
                {
                    try { tileIds.push_back(std::stoi(token)); }
                    catch (...) {}
                }
                if (!tileIds.empty())
                    fresh.GenerateCollisionData(s_collisionLayer, tileIds);
            }

            EntityID entity = static_cast<EntityID>(m_ctx->selectedEntity);
            m_ecs->RemoveComponent<Tilemap2D>(entity);
            m_ecs->AddComponent<Tilemap2D>(entity, std::move(fresh));

            s_seededFor = std::string(s_mapPathBuf);
        };

    // stats
    BeginPropertyTable();

    char dimsBuf[64];
    std::snprintf(dimsBuf, sizeof(dimsBuf), "%d x %d", tm.GetMapWidth(), tm.GetMapHeight());
    NextRow(); FieldReadOnly("Map size", dimsBuf);

    char collisionBuf[32];
    std::snprintf(collisionBuf, sizeof(collisionBuf), "%zu regions", tm.GetCollisionRegions().size());
    NextRow(); FieldReadOnly("Collision", collisionBuf);

    char childrenBuf[32];
    std::snprintf(childrenBuf, sizeof(childrenBuf), "%zu entities", tm.GetChildEntities().size());
    NextRow(); FieldReadOnly("Children", childrenBuf);

    EndPropertyTable();

    // layers
    if (!tm.GetLayers().empty() && ImGui::TreeNode("Layers"))
    {
        for (size_t i = 0; i < tm.GetLayers().size(); i++)
        {
            const auto& layer = tm.GetLayers()[i];
            ImGui::BulletText("[%zu] %s", i, layer.name.empty() ? "(unnamed)" : layer.name.c_str());
        }
        ImGui::TreePop();
    }

    // map
    if (ImGui::TreeNodeEx("Map", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool changed = false;

        BeginPropertyTable();

        NextRow();
        ImGui::PushID("mappath");
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Path");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::InputText("##v", s_mapPathBuf, sizeof(s_mapPathBuf),
            ImGuiInputTextFlags_EnterReturnsTrue))
            changed = true;
        ImGui::SameLine();
        if (ImGui::SmallButton("...##map"))
        {
            m_pendingPicker = {
                "##pickerMap", ".json",
                [](const std::string& path)
                {
                    std::strncpy(s_mapPathBuf, path.c_str(), sizeof(s_mapPathBuf) - 1);
                    s_rebuildPending = true;
                }
            };
        }
        ImGui::PopID();

        NextRow(); FieldInt("Tile W", &s_tileW, 1, 1, 0);
        if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;

        NextRow(); FieldInt("Tile H", &s_tileH, 1, 1, 0);
        if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;

        EndPropertyTable();

        if (changed) tryRebuild();

        ImGui::TreePop();
    }

    // tilesets
    if (ImGui::TreeNodeEx("Tilesets", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int removeIdx = -1;
        for (int i = 0; i < (int)s_tilesets.size(); i++)
        {
            ImGui::PushID(i);
            ImGui::BulletText("[gid %d] %s", s_tilesets[i].firstgid,
                s_tilesets[i].texturePath.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("x")) removeIdx = i;
            ImGui::PopID();
        }

        if (removeIdx >= 0)
        {
            s_tilesets.erase(s_tilesets.begin() + removeIdx);
            tryRebuild();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        BeginPropertyTable();

        NextRow();
        ImGui::PushID("pendingTex");
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Texture");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
        ImGui::InputText("##v", s_pendingPath, sizeof(s_pendingPath));
        ImGui::SameLine();
        if (ImGui::SmallButton("...##ts"))
        {
            m_pendingPicker =
            {
                "##pickerTileset", ".png",
                [](const std::string& path)
                {
                    std::strncpy(s_pendingPath, path.c_str(), sizeof(s_pendingPath) - 1);
                }
            };
        }
        ImGui::PopID();

        NextRow(); FieldInt("First GID", &s_pendingFirstgid, 1, -1, 0);

        EndPropertyTable();
        ImGui::TextDisabled("First GID: -1 = auto");

        if (ImGui::Button("Add") && s_pendingPath[0] != '\0')
        {
            s_tilesets.push_back({ std::string(s_pendingPath), s_pendingFirstgid });
            s_pendingPath[0] = '\0';
            s_pendingFirstgid = -1;
            tryRebuild();
        }

        ImGui::TreePop();
    }

    // collision
    if (ImGui::TreeNodeEx("Collision", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool changed = false;

        BeginPropertyTable();

        NextRow(); FieldInt("Layer", &s_collisionLayer, 1, 0, 0);
        if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;

        NextRow();
        ImGui::PushID("gids");
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("GIDs");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("##v", s_tileGidsBuf, sizeof(s_tileGidsBuf),
            ImGuiInputTextFlags_EnterReturnsTrue))
            changed = true;
        ImGui::PopID();

        EndPropertyTable();
        ImGui::TextDisabled("Comma-separated e.g. 1,2,5");

        if (changed) tryRebuild();

        ImGui::TreePop();
    }

    if (s_rebuildPending)
    {
        s_rebuildPending = false;
        tryRebuild();
    }

    EndComponent();

    if (removeRequested)
    {
        m_ecs->RemoveComponent<Tilemap2D>(static_cast<EntityID>(m_ctx->selectedEntity));
        s_seededFor.clear();
        s_tilesets.clear();
        s_mapPathBuf[0] = '\0';
        s_pendingPath[0] = '\0';
    }
}

void InspectorPanel::drawBehaviorScript(BehaviorScript& bs)
{
    bool removeRequested = false;
    ImTextureID iconHandle = m_iconScript ? (ImTextureID)(intptr_t)m_iconScript->GetHandle() : 0;
    if (!BeginComponent("BehaviorScript", &removeRequested, iconHandle)) return;

    if (bs.scriptPath.empty())
    {
        static char s_dirBuf[512] = {};
        static char s_classNameBuf[128] = {};

        BeginPropertyTable();

        // directory row — browse button opens folder picker
        NextRow();
        ImGui::PushID("scriptdir");
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Directory");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
        ImGui::InputText("##dir", s_dirBuf, sizeof(s_dirBuf));
        ImGui::SameLine();
        if (ImGui::SmallButton("...##dir"))
        {
            m_pendingPicker = {
                "##pickerBehaviorDir", "",  // no extension filter
                [](const std::string& path)
                {
                    // strip filename
                    size_t slash = path.find_last_of("/\\");
                    std::string dir = (slash != std::string::npos) ? path.substr(0, slash) : path;
                    std::strncpy(s_dirBuf, dir.c_str(), sizeof(s_dirBuf) - 1);
                }
            };
        }
        ImGui::PopID();

        // class name row
        NextRow();
        ImGui::PushID("scriptname");
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Class Name");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##name", s_classNameBuf, sizeof(s_classNameBuf));
        ImGui::PopID();

        // full path preview
        if (s_dirBuf[0] != '\0' && s_classNameBuf[0] != '\0')
        {
            char previewBuf[640];
            std::snprintf(previewBuf, sizeof(previewBuf), "%s/%s", s_dirBuf, s_classNameBuf);
            NextRow(); FieldReadOnly("Full Path", previewBuf);
        }

        EndPropertyTable();

        ImGui::TextDisabled("Class name only, no extension");

        bool canCreate = s_dirBuf[0] != '\0' && s_classNameBuf[0] != '\0';
        if (!canCreate) ImGui::BeginDisabled();

        if (ImGui::Button("Create & Bind", ImVec2(-1.0f, 0.0f)))
        {
            std::string fullPath = std::string(s_dirBuf) + "/" + std::string(s_classNameBuf);
            std::string className = std::string(s_classNameBuf);

            generateBehaviorStub(fullPath, className);

            bs.scriptPath = fullPath;
            bs.behaviorName = className;
            s_dirBuf[0] = '\0';
            s_classNameBuf[0] = '\0';
        }

        if (!canCreate) ImGui::EndDisabled();
    }
    else
    {
        // script bound
        BeginPropertyTable();
        NextRow(); FieldReadOnly("Script", bs.scriptPath.c_str());
        EndPropertyTable();

        if (ImGui::Button("Clear", ImVec2(-1.0f, 0.0f)))
        {
            bs.scriptPath.clear();
            bs.behaviorName.clear();
        }
    }

    EndComponent();

    if (removeRequested)
    {
        m_ecs->RemoveComponent<BehaviorScript>(static_cast<EntityID>(m_ctx->selectedEntity));
    }
}

void InspectorPanel::generateBehaviorStub(const std::string& path, const std::string& className)
{
    std::string hPath = path + ".h";
    std::string cppPath = path + ".cpp";

    // Do not overwrite existing files
    if (std::ifstream(hPath) || std::ifstream(cppPath))
    {
        Debug::LogWarn("Behavior stub already exists at: %s", path.c_str());
        return;
    }

    std::ofstream h(hPath);
    if (h.is_open())
    {
        h << "#pragma once\n";
        h << "#include <Core/Behavior/Behavior.h>\n\n";
        h << "class " << className << " : public Nova::Behavior\n";
        h << "{\n";
        h << "public:\n";
        h << "    const char* GetName() const override { return \"" << className << "\"; }\n\n";
        h << "    void OnStart()          override {}\n";
        h << "    void OnUpdate(float dt) override {}\n";
        h << "    void OnDestroy()        override {}\n";
        h << "};\n";
    }

    std::ofstream cpp(cppPath);
    if (cpp.is_open())
    {
        cpp << "#include \"" << className << ".h\"\n";
        cpp << "#include <Core/Behavior/BehaviorRegistry.h>\n\n";
        cpp << "REGISTER_BEHAVIOR(" << className << ")\n";
    }

    Debug::LogInfo("[Editor] Generated behavior stub: %s", path.c_str());
}
