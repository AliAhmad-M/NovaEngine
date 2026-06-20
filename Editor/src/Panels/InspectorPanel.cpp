#include <Editor/Panels/InspectorPanel.h>
#include <Core/ECS/ECS.h>
#include <Core/Services.h>
#include <Graphics/Texture.h>
#include <imgui.h>
#include <cstring>
#include <cmath>
#include <array>

using namespace Nova::Editor;

static void FieldVec2(const char* label, float* v, float speed = 0.1f)
{
    ImGui::PushID(label);
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::DragFloat2("##v", v, speed);
    ImGui::PopID();
}

static void FieldFloat(const char* label, float* v, float speed = 0.01f,
    float vmin = 0.0f, float vmax = 0.0f)
{
    ImGui::PushID(label);
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::DragFloat("##v", v, speed, vmin, vmax);
    ImGui::PopID();
}

static void FieldInt(const char* label, int* v, float speed = 1.0f, int vmin = 0, int vmax = 0)
{
    ImGui::PushID(label);
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::DragInt("##v", v, speed, vmin, vmax);
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

static bool BeginComponent(const char* name, bool* removeRequested = nullptr)
{
    ImGui::PushID(name);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_AllowOverlap |
        ImGuiTreeNodeFlags_FramePadding;

    bool open = ImGui::TreeNodeEx(name, flags);

    if (removeRequested)
    {
        float btnW = ImGui::GetFrameHeight();
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - btnW + ImGui::GetCursorPosX()
            - ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("x", ImVec2(btnW, btnW))) *removeRequested = true;
    }

    if (!open)
    {
        ImGui::PopID();
    }

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

InspectorPanel::InspectorPanel(Context* ctx) : m_ctx(ctx) {}

void InspectorPanel::Draw()
{
    ImGui::Begin("Inspector");

    if (m_ctx->selectedEntity == -1)
    {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    ECS* ecs = Services::Fetch<ECS>();
    if (!ecs)
    {
        ImGui::TextDisabled("No ECS available");
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

    // Draw each component section if the entity has that component
    if (Transform2D* t = ecs->GetComponent<Transform2D>(entity))
        drawTransform2D(*t);

    if (Camera2D* c = ecs->GetComponent<Camera2D>(entity))
        drawCamera2D(*c);

    if (Sprite2D* s = ecs->GetComponent<Sprite2D>(entity))
        drawSprite2D(*s);

    if (RigidBody2D* rb = ecs->GetComponent<RigidBody2D>(entity))
        drawRigidBody2D(*rb);

    if (Tilemap2D* tm = ecs->GetComponent<Tilemap2D>(entity))
        drawTilemap2D(*tm);

    ImGui::Spacing();
    drawAddComponentMenu();

    ImGui::End();
}

void InspectorPanel::drawTransform2D(Transform2D& t)
{
    bool removeRequested = false;
    if (!BeginComponent("Transform2D", &removeRequested)) return;

    BeginPropertyTable();
    NextRow(); FieldVec2("Position", &t.position.x);
    NextRow(); FieldFloat("Rotation", &t.rotation, 0.5f);
    NextRow(); FieldVec2("Scale", &t.scale.x);
    EndPropertyTable();

    EndComponent();

    if (removeRequested)
    {
        ECS* ecs = Services::Fetch<ECS>();
        ecs->RemoveComponent<Transform2D>(static_cast<EntityID>(m_ctx->selectedEntity));
    }
}

void InspectorPanel::drawCamera2D(Camera2D& c)
{
    bool removeRequested = false;
    if (!BeginComponent("Camera2D", &removeRequested)) return;

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

    // Boundaries: left, right, up, down. Infinite values are edited as
    // large finite numbers in the UI and converted back to INFINITY on commit.
    std::array<float, 4> boundaries = c.GetBoundaries();
    bool boundsChanged = false;

    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Bound Left");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    {
        bool unbounded = std::isinf(boundaries[0]);
        float v = unbounded ? 0.0f : boundaries[0];
        ImGui::PushID("BoundLeft");
        if (unbounded) ImGui::BeginDisabled();
        if (ImGui::DragFloat("##v", &v, 1.0f)) { boundaries[0] = v; boundsChanged = true; }
        if (unbounded) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Checkbox("∞##leftInf", &unbounded))
        {
            boundaries[0] = unbounded ? -INFINITY : 0.0f;
            boundsChanged = true;
        }
        ImGui::PopID();
    }

    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Bound Right");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    {
        bool unbounded = std::isinf(boundaries[1]);
        float v = unbounded ? 0.0f : boundaries[1];
        ImGui::PushID("BoundRight");
        if (unbounded) ImGui::BeginDisabled();
        if (ImGui::DragFloat("##v", &v, 1.0f)) { boundaries[1] = v; boundsChanged = true; }
        if (unbounded) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Checkbox("∞##rightInf", &unbounded))
        {
            boundaries[1] = unbounded ? INFINITY : 0.0f;
            boundsChanged = true;
        }
        ImGui::PopID();
    }

    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Bound Up");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    {
        bool unbounded = std::isinf(boundaries[2]);
        float v = unbounded ? 0.0f : boundaries[2];
        ImGui::PushID("BoundUp");
        if (unbounded) ImGui::BeginDisabled();
        if (ImGui::DragFloat("##v", &v, 1.0f)) { boundaries[2] = v; boundsChanged = true; }
        if (unbounded) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Checkbox("∞##upInf", &unbounded))
        {
            boundaries[2] = unbounded ? -INFINITY : 0.0f;
            boundsChanged = true;
        }
        ImGui::PopID();
    }

    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Bound Down");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    {
        bool unbounded = std::isinf(boundaries[3]);
        float v = unbounded ? 0.0f : boundaries[3];
        ImGui::PushID("BoundDown");
        if (unbounded) ImGui::BeginDisabled();
        if (ImGui::DragFloat("##v", &v, 1.0f)) { boundaries[3] = v; boundsChanged = true; }
        if (unbounded) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Checkbox("∞##downInf", &unbounded))
        {
            boundaries[3] = unbounded ? INFINITY : 0.0f;
            boundsChanged = true;
        }
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

    const Vector2& offset = c.GetOffset();
    char offsetBuf[64];
    std::snprintf(offsetBuf, sizeof(offsetBuf), "(%.2f, %.2f)", offset.x, offset.y);
    NextRow(); FieldReadOnly("Offset", offsetBuf);

    EndPropertyTable();
    EndComponent();

    if (removeRequested)
    {
        ECS* ecs = Services::Fetch<ECS>();
        ecs->RemoveComponent<Camera2D>(static_cast<EntityID>(m_ctx->selectedEntity));
    }
}

void InspectorPanel::drawTexturePathField(Sprite2D& s)
{
    // Get path to the texture
    static char pathBuf[256];
    std::string currentPath = Services::Fetch<ContentLoader>()->GetTexturePath(s.texture);
    std::snprintf(pathBuf, sizeof(pathBuf), "%s", currentPath.c_str());

    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Texture");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-28.0f);

    if (ImGui::InputText("##texPath", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        Texture* loaded = Services::Fetch<ContentLoader>()->LoadTexture(pathBuf);
        if (loaded) s.texture = loaded;
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("..."))
    {
        // TODO: open Asset Browser picker filtered to AssetType::Texture
    }
}

void InspectorPanel::drawSprite2D(Sprite2D& s)
{
    bool removeRequested = false;
    if (!BeginComponent("Sprite2D", &removeRequested)) return;

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
    int zIndex = s.GetZIndex();
    if (ImGui::DragInt("##zindex", &zIndex))
        s.SetZIndex(zIndex);

    EndPropertyTable();
    EndComponent();

    if (removeRequested)
    {
        ECS* ecs = Services::Fetch<ECS>();
        ecs->RemoveComponent<Sprite2D>(static_cast<EntityID>(m_ctx->selectedEntity));
    }
}

void InspectorPanel::drawRigidBody2D(RigidBody2D& rb)
{
    bool removeRequested = false;
    if (!BeginComponent("RigidBody2D", &removeRequested)) return;

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
    if (isStatic != rb.IsStatic())
        rb.SetStatic(isStatic);

    NextRow();
    bool isSensor = rb.IsSensor();
    FieldBool("Sensor", &isSensor);
    if (isSensor != rb.IsSensor())
        rb.SetSensor(isSensor);

    NextRow();
    float density = rb.GetDensity();
    FieldFloat("Density", &density, 0.01f, 0.0f, 0.0f);
    if (density != rb.GetDensity())
        rb.SetDensity(density);

    NextRow();
    float restitution = rb.GetRestitution();
    FieldFloat("Restitution", &restitution, 0.01f, 0.0f, 1.0f);
    if (restitution != rb.GetRestitution())
        rb.SetRestitution(restitution);

    NextRow();
    float friction = rb.GetFriction();
    FieldFloat("Friction", &friction, 0.01f, 0.0f, 1.0f);
    if (friction != rb.GetFriction())
        rb.SetFriction(friction);

    NextRow();
    bool lockRotation = rb.LockRotation();
    FieldBool("Lock Rotation", &lockRotation);
    if (lockRotation != rb.LockRotation())
        rb.SetLockRotation(lockRotation);

    NextRow();
    float linearDamping = rb.GetLinearDamping();
    FieldFloat("Linear Damping", &linearDamping, 0.01f, 0.0f, 0.0f);
    if (linearDamping != rb.GetLinearDamping())
        rb.SetLinearDamping(linearDamping);

    NextRow();
    float angularDamping = rb.GetAngularDamping();
    FieldFloat("Angular Damping", &angularDamping, 0.01f, 0.0f, 0.0f);
    if (angularDamping != rb.GetAngularDamping())
        rb.SetAngularDamping(angularDamping);

    EndPropertyTable();
    EndComponent();

    if (removeRequested)
    {
        ECS* ecs = Services::Fetch<ECS>();
        ecs->RemoveComponent<RigidBody2D>(static_cast<EntityID>(m_ctx->selectedEntity));
    }
}

void InspectorPanel::drawTilemap2D(Tilemap2D& tm)
{
    bool removeRequested = false;
    if (!BeginComponent("Tilemap2D", &removeRequested)) return;

    // Tilemap2D is populated via LoadTilemap()/AddTileset() rather than
    // per-field setters, so this is primarily a read-only diagnostic view.
    BeginPropertyTable();

    char dimsBuf[64];
    std::snprintf(dimsBuf, sizeof(dimsBuf), "%d x %d tiles", tm.GetMapWidth(), tm.GetMapHeight());
    NextRow(); FieldReadOnly("Map Size", dimsBuf);

    char tileSizeBuf[64];
    std::snprintf(tileSizeBuf, sizeof(tileSizeBuf), "%d x %d px", tm.GetTileWidth(), tm.GetTileHeight());
    NextRow(); FieldReadOnly("Tile Size", tileSizeBuf);

    const Vector2& offset = tm.GetOffset();
    char offsetBuf[64];
    std::snprintf(offsetBuf, sizeof(offsetBuf), "(%.2f, %.2f)", offset.x, offset.y);
    NextRow(); FieldReadOnly("Offset", offsetBuf);

    char tilesetBuf[32];
    std::snprintf(tilesetBuf, sizeof(tilesetBuf), "%zu tiles", tm.GetTileset().size());
    NextRow(); FieldReadOnly("Tileset", tilesetBuf);

    char layersBuf[32];
    std::snprintf(layersBuf, sizeof(layersBuf), "%zu", tm.GetLayers().size());
    NextRow(); FieldReadOnly("Layers", layersBuf);

    char collisionBuf[32];
    std::snprintf(collisionBuf, sizeof(collisionBuf), "%zu regions", tm.GetCollisionRegions().size());
    NextRow(); FieldReadOnly("Collision", collisionBuf);

    char childrenBuf[32];
    std::snprintf(childrenBuf, sizeof(childrenBuf), "%zu entities", tm.GetChildEntities().size());
    NextRow(); FieldReadOnly("Children", childrenBuf);

    EndPropertyTable();

    if (!tm.GetLayers().empty() && ImGui::TreeNode("Layer Names"))
    {
        for (size_t i = 0; i < tm.GetLayers().size(); i++)
        {
            const auto& layer = tm.GetLayers()[i];
            ImGui::BulletText("[%zu] %s", i, layer.name.empty() ? "(unnamed)" : layer.name.c_str());
        }
        ImGui::TreePop();
    }

    EndComponent();

    if (removeRequested)
    {
        ECS* ecs = Services::Fetch<ECS>();
        ecs->RemoveComponent<Tilemap2D>(static_cast<EntityID>(m_ctx->selectedEntity));
    }
}

void InspectorPanel::drawAddComponentMenu()
{
    ECS* ecs = Services::Fetch<ECS>();
    EntityID entity = static_cast<EntityID>(m_ctx->selectedEntity);

    float btnW = 200.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - btnW) * 0.5f);
    if (ImGui::Button("Add Component", ImVec2(btnW, 0)))
        ImGui::OpenPopup("AddComponentPopup");

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        ImGui::TextDisabled("Components");
        ImGui::Separator();

        if (!ecs->HasComponent<Transform2D>(entity) && ImGui::MenuItem("Transform2D"))
            ecs->AddComponent<Transform2D>(entity, Transform2D());

        if (!ecs->HasComponent<Camera2D>(entity) && ImGui::MenuItem("Camera2D"))
            ecs->AddComponent<Camera2D>(entity, Camera2D());

        if (!ecs->HasComponent<Sprite2D>(entity) && ImGui::MenuItem("Sprite2D"))
            ecs->AddComponent<Sprite2D>(entity, Sprite2D());
        if (!ecs->HasComponent<RigidBody2D>(entity) && ImGui::MenuItem("RigidBody2D (Circle)"))
            ecs->AddComponent<RigidBody2D>(entity, RigidBody2D(Vector2::ZERO, PHYSICS_PPM));
        if (!ecs->HasComponent<RigidBody2D>(entity) && ImGui::MenuItem("RigidBody2D (Box)"))
            ecs->AddComponent<RigidBody2D>(entity, RigidBody2D(Vector2::ZERO, PHYSICS_PPM, PHYSICS_PPM));

        if (!ecs->HasComponent<Tilemap2D>(entity) && ImGui::MenuItem("Tilemap2D"))
            ecs->AddComponent<Tilemap2D>(entity, Tilemap2D());

        ImGui::EndPopup();
    }
}