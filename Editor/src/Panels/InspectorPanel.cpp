#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Panels/InspectorPanel.h>
#include <Core/ECS/ECS.h>
#include <Core/Behavior/BehaviorRegistry.h>
#include <Core/Services.h>
#include <Graphics/Texture.h>
#include <Editor/Inspector/EditorFields.h>
#include <Editor/FilePicker.h>
#include <Editor/Panels/ScenePanel.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <cstring>
#include <cmath>
#include <array>
#include <sstream>
#include <vector>

using namespace Nova::Editor;

InspectorPanel::InspectorPanel(EditorContext* ctx) : m_ctxRef(ctx)
{
    m_ecsRef = Services::Fetch<ECS>();
    m_contentLoaderRef = Services::Fetch<ContentLoader>();

    m_iconTransform  = m_contentLoaderRef->LoadTexture("assets/icons/move.png");
    m_iconSprite     = m_contentLoaderRef->LoadTexture("assets/icons/image.png");
    m_iconCamera     = m_contentLoaderRef->LoadTexture("assets/icons/icon_camera.png");
    m_iconRigidbody  = m_contentLoaderRef->LoadTexture("assets/icons/atom.png");
    m_iconTilemap    = m_contentLoaderRef->LoadTexture("assets/icons/grid.png");
    m_iconScript     = m_contentLoaderRef->LoadTexture("assets/icons/script.png");
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

template<typename T>
void drawComponent(
    const char* label,
    T& component, ImTextureID icon,
    std::function<void()> onRemove,
    EditorFields::PickerFn picker = nullptr)
{
    bool removeRequested = false;
    if (!BeginComponent(label, &removeRequested, icon)) return;

    BeginPropertyTable();
    EditorFields visitor;
    visitor.requestPicker = picker;
    component.ReflectFields(visitor);
    EndPropertyTable();
    EndComponent();

    if (removeRequested) onRemove();
}

void InspectorPanel::Draw()
{
    ImGui::Begin("Inspector");

    if (m_ctxRef->selectedEntity == -1)
    {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    EntityID entity = static_cast<EntityID>(m_ctxRef->selectedEntity);

    char nameLabel[64];
    std::snprintf(nameLabel, sizeof(nameLabel), "Entity %d", m_ctxRef->selectedEntity);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::Text("%s", nameLabel);
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::Spacing();

    ImGuiContext& g = *ImGui::GetCurrentContext();
    const bool editedBefore = g.ActiveIdHasBeenEditedThisFrame;

    // draw each component section if the entity has that component
    if (Transform2D* t = m_ecsRef->GetComponent<Transform2D>(entity))
    {
        ImTextureID iconHandle = m_iconTransform ? (ImTextureID)(intptr_t)m_iconTransform->GetHandle() : 0;
        drawComponent("Transform2D", *t, iconHandle,
            [&] { m_ecsRef->RemoveComponent<Transform2D>(entity); });
    }

    if (Camera2D* c = m_ecsRef->GetComponent<Camera2D>(entity))
    {
        ImTextureID iconHandle = m_iconCamera ? (ImTextureID)(intptr_t)m_iconCamera->GetHandle() : 0;
		drawComponent("Camera2D", *c, iconHandle,
			[&] { m_ecsRef->RemoveComponent<Camera2D>(entity); });
    }

    if (Sprite2D* s = m_ecsRef->GetComponent<Sprite2D>(entity))
    {
        ImTextureID iconHandle = m_iconSprite ? (ImTextureID)(intptr_t)m_iconSprite->GetHandle() : 0;

        // Sync texture
        std::string before = s->texture ? m_contentLoaderRef->GetTexturePath(s->texture) : "";
        s->texturePath = before;

        // Picker request
        auto picker = [this, entity](const char* id, const char* ext,
            std::function<void(const std::string&)>)
            {
                m_pendingPicker = {
                    id, ext,
                    [this, entity](const std::string& picked)
                    {
                        Sprite2D* sp = m_ecsRef->GetComponent<Sprite2D>(entity);
                        if (!sp) return;

                        Texture* loaded = m_contentLoaderRef->LoadTexture(picked);
                        if (loaded)
                        {
                            sp->texture = loaded;
                            sp->texturePath = picked;
                        }
                    }
                };
            };

        drawComponent("Sprite2D", *s, iconHandle,
            [&] { m_ecsRef->RemoveComponent<Sprite2D>(entity); },
            picker);

        // Fetch component again
        s = m_ecsRef->GetComponent<Sprite2D>(entity);
        if (s)
        {
            // Typed path
            if (s->texturePath != before)
            {
                Texture* loaded = m_contentLoaderRef->LoadTexture(s->texturePath);
                if (loaded) s->texture = loaded;
                else        s->texturePath = before;
            }

            s->hframes = std::max(s->hframes, 1);
            s->vframes = std::max(s->vframes, 1);
        }
    }
 
    if (RigidBody2D* rb = m_ecsRef->GetComponent<RigidBody2D>(entity))
    {
        ImTextureID iconHandle = m_iconRigidbody ? (ImTextureID)(intptr_t)m_iconRigidbody->GetHandle() : 0;
        drawComponent("RigidBody2D", *rb, iconHandle,
            [&] { m_ecsRef->RemoveComponent<RigidBody2D>(entity); });
    }

    if (Tilemap2D* tm = m_ecsRef->GetComponent<Tilemap2D>(entity))
    {
        ImTextureID iconHandle = m_iconTilemap ? (ImTextureID)(intptr_t)m_iconTilemap->GetHandle() : 0;

		// Open file picker (next frame) and mark dirty (this frame) when adding path
        auto picker = [this, entity, tm](const char* id, const char* ext,
            std::function<void(const std::string&)> cb)
            {
                m_pendingPicker = {
                    id, ext,
                    [this, entity, tm, cb](const std::string& picked)
                    {
                        Tilemap2D* cur = m_ecsRef->GetComponent<Tilemap2D>(entity);
                        if (cur != tm) return;

                        cb(picked);
                        cur->MarkRebuildNeeded();
                    }
                };
            };

        drawComponent("Tilemap2D", *tm, iconHandle,
            [&] { m_ecsRef->RemoveComponent<Tilemap2D>(entity); }, picker);

        // Fetch component again
        tm = m_ecsRef->GetComponent<Tilemap2D>(entity);

        // Rebuild it if needed
        if (tm && tm->NeedsRebuild() && !ImGui::IsAnyItemActive()) 
        {
            Tilemap2D* old = m_ecsRef->GetComponent<Tilemap2D>(entity);

            // Clear first so a failed build isn't retried every frame
            old->ClearRebuildFlag();

            // Copy everything out, the old component is destroyed below
            std::string mapPath = old->GetSourceMapPath();
            std::vector<TilesetSource2D> tilesets = old->GetTilesetSources();
            int tileW = old->GetTileWidth();
            int tileH = old->GetTileHeight();
            int collisionLayer = old->collisionLayer;
            std::string collisionGids = old->collisionGids;
            std::string pendingTex = old->pendingTexturePath;
            int pendingGid = old->pendingFirstgid;

            if (tilesets.empty() || mapPath.empty()) return;

            Tilemap2D fresh(tileW, tileH);

            for (const auto& src : tilesets)
            {
                const Texture* tex = m_contentLoaderRef->LoadTexture(src.texturePath);
                if (!tex)
                {
                    Debug::LogError("[Tilemap] Failed to load tileset: %s", src.texturePath.c_str());
                    return;
                }
                fresh.AddTileset(tex, src.texturePath, src.firstgid);
            }

            fresh.LoadTilemap(mapPath);
            fresh.SetSourceMapPath(mapPath);

            if (!collisionGids.empty())
            {
                std::vector<int> tileIds;
                std::istringstream ss(collisionGids);
                std::string token;
                while (std::getline(ss, token, ','))
                {
                    try { tileIds.push_back(std::stoi(token)); }
                    catch (...) {}
                }
                if (!tileIds.empty())
                    fresh.GenerateCollisionData(collisionLayer, tileIds);
            }

            fresh.collisionLayer = collisionLayer;
            fresh.collisionGids = collisionGids;
            fresh.pendingTexturePath = pendingTex;
            fresh.pendingFirstgid = pendingGid;

            m_ecsRef->RemoveComponent<Tilemap2D>(entity);
            m_ecsRef->AddComponent<Tilemap2D>(entity, std::move(fresh));
        }
    }

    if (BehaviorScript* bs = m_ecsRef->GetComponent<BehaviorScript>(entity))
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
        std::string path = Services::Fetch<ProjectManager>()->Current().root.string();
        if (!DrawFilePicker(req.popupId, path, req.filterExt, req.onConfirm))
        {
            m_pendingPicker.reset();
            m_pickerOpen = false;
        }
    }

    // If components have been modified, mark scene as dirty
    if (!editedBefore && g.ActiveIdHasBeenEditedThisFrame)
    {
        if (m_ctxRef->scenePanelRef)
			m_ctxRef->scenePanelRef->MarkSceneAsDirty();
    }

    ImGui::End();
}

void InspectorPanel::drawAddComponentMenu()
{
    EntityID entity = static_cast<EntityID>(m_ctxRef->selectedEntity);

    float btnW = 200.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - btnW) * 0.5f);
    if (ImGui::Button("Add Component", ImVec2(btnW, 0)))
        ImGui::OpenPopup("AddComponentPopup");

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        ImGui::TextDisabled("Components");
        ImGui::Separator();

        if (!m_ecsRef->HasComponent<Transform2D>(entity) && ImGui::MenuItem("Transform2D"))
            m_ecsRef->AddComponent<Transform2D>(entity, Transform2D());

        if (!m_ecsRef->HasComponent<Camera2D>(entity) && ImGui::MenuItem("Camera2D"))
            m_ecsRef->AddComponent<Camera2D>(entity, Camera2D());

        if (!m_ecsRef->HasComponent<Sprite2D>(entity) && ImGui::MenuItem("Sprite2D"))
            m_ecsRef->AddComponent<Sprite2D>(entity, Sprite2D());

        if (!m_ecsRef->HasComponent<RigidBody2D>(entity) && ImGui::MenuItem("RigidBody2D (Circle)"))
            m_ecsRef->AddComponent<RigidBody2D>(entity, RigidBody2D(Vector2::ZERO, PHYSICS_PPM));

        if (!m_ecsRef->HasComponent<RigidBody2D>(entity) && ImGui::MenuItem("RigidBody2D (Box)"))
            m_ecsRef->AddComponent<RigidBody2D>(entity, RigidBody2D(Vector2::ZERO, PHYSICS_PPM, PHYSICS_PPM));

        if (!m_ecsRef->HasComponent<Tilemap2D>(entity) && ImGui::MenuItem("Tilemap2D"))
            m_ecsRef->AddComponent<Tilemap2D>(entity, Tilemap2D());

        if (!m_ecsRef->HasComponent<BehaviorScript>(entity) && ImGui::MenuItem("Script"))
            m_ecsRef->AddComponent<BehaviorScript>(entity, BehaviorScript());

        ImGui::EndPopup();
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

        // directory row
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
        NextRow(); FieldReadOnly("Name", bs.behaviorName.c_str());
        NextRow(); FieldReadOnly("Path", (bs.scriptPath + ".hpp").c_str());
        EndPropertyTable();

        if (ImGui::Button("Clear", ImVec2(-1.0f, 0.0f)))
        {
            bs.scriptPath.clear();
            bs.behaviorName.clear();
        }
    }

    if (bs.instance)
    {
        ImGui::Separator();
        EditorFields visitor;
        bs.instance->ReflectFields(visitor);
    }

    EndComponent();

    if (removeRequested)
    {
        m_ecsRef->RemoveComponent<BehaviorScript>(static_cast<EntityID>(m_ctxRef->selectedEntity));
    }
}

void InspectorPanel::generateBehaviorStub(const std::string& path, const std::string& className)
{
    std::string hppPath = path + ".hpp";
    std::string cppPath = path + ".cpp";

    // Do not overwrite existing files
    if (std::ifstream(hppPath) || std::ifstream(cppPath))
    {
        Debug::LogWarn("Behavior stub already exists at: %s", path.c_str());
        return;
    }

    std::ofstream hpp(hppPath);
    if (hpp.is_open())
    {
        hpp << "#pragma once\n";
        hpp << "#include <Core/Behavior/Behavior.h>\n\n";
        hpp << "class " << className << " : public Nova::Behavior\n";
        hpp << "{\n";
        hpp << "public:\n";
        hpp << "    const char* GetName() const override { return \"" << className << "\"; }\n\n";
        hpp << "    void OnStart()          override {}\n";
        hpp << "    void OnUpdate(float dt) override {}\n";
        hpp << "    void OnDestroy()        override {}\n";
        hpp << "};\n";
    }

    std::ofstream cpp(cppPath);
    if (cpp.is_open())
    {
        cpp << "#include \"" << className << ".hpp\"\n";
        cpp << "#include <Core/Behavior/BehaviorRegistry.h>\n\n";
        cpp << "REGISTER_BEHAVIOR(" << className << ")\n";
    }

    Debug::LogInfo("[Editor] Generated behavior stub: %s.hpp / %s.cpp", path.c_str(), path.c_str());
}
