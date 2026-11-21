#define _CRT_SECURE_NO_WARNINGS
#include <Editor/Panels/InspectorPanel.h>
#include <Editor/Theme.h>
#include <Core/ECS/ECS.h>
#include <Core/Behavior/BehaviorRegistry.h>
#include <Core/Services.h>
#include <Graphics/Texture.h>
#include <Editor/Inspector/EditorFields.h>
#include <Editor/FilePicker.h>
#include <Editor/AssetPath.h>
#include <Editor/Panels/ScenePanel.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <cstring>
#include <cmath>
#include <array>
#include <sstream>
#include <vector>
#include <algorithm>
#include <filesystem>

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

static void PropLabel(const char* label)
{
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.68f, 0.70f, 0.74f, 1.0f));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::TableSetColumnIndex(1);
}

static void FieldReadOnly(const char* label, const char* value)
{
    ImGui::PushID(label);
    PropLabel(label);
    ImGui::TextDisabled("%s", value);
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

    // Draw the arrow/frame only
    ImGui::PushStyleColor(ImGuiCol_Header, Theme::Raised());
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Theme::RaisedHover());
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, Theme::RaisedHover());
    bool open = ImGui::TreeNodeEx("##header", flags);
    ImGui::PopStyleColor(3);
    
    float lineHeight = ImGui::GetFrameHeight();
    float textHeight = ImGui::GetTextLineHeight();

    // Icon comes after the arrow
    ImGui::SameLine();
    if (icon)
    {
        float iconSize = 16.0f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (lineHeight - iconSize) * 0.5f);
        ImGui::Image(icon, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1));
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (lineHeight - iconSize) * 0.5f);
    }

    // Label after the icon
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (lineHeight - textHeight) * 0.5f);
    ImGui::TextUnformatted(name);
    

    // Button with 3 dots
    float btnW = ImGui::GetFrameHeight() - 2.0f;
    float rightEdge = ImGui::GetWindowContentRegionMax().x;
    ImGui::SameLine(rightEdge - btnW);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.25f));
    if (ImGui::Button("...##opts", ImVec2(btnW, btnW)))
        ImGui::OpenPopup("##ComponentCtx");
    ImGui::PopStyleColor(3);

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
    ImGui::Spacing();
}

static void BeginPropertyTable()
{
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 4));
    ImGui::BeginTable("##props", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 110.0f);
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

    if (m_ctxRef->selectedEntity == -1 || !m_ctxRef->activeSceneRef)
    {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    ProjectManager* pm = Services::Fetch<ProjectManager>();
    const std::filesystem::path projectRoot = (pm && pm->HasProject()) ? pm->Current().root : std::filesystem::path();

    EntityID entity = static_cast<EntityID>(m_ctxRef->selectedEntity);

    std::string entityName = "Entity";
    if (m_ctxRef->activeSceneRef)
    {
        for (auto& e : m_ctxRef->activeSceneRef->GetEntities())
        {
            if (e.id == entity)
            {
                entityName = e.name;
                break;
            }
        }
    }

    char nameBuf[128];
    std::strncpy(nameBuf, entityName.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 70.0f);
    ImGui::InputText("##entityName", nameBuf, sizeof(nameBuf));
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        for (auto& e : m_ctxRef->activeSceneRef->GetEntities())
        {
            if (e.id == entity)
            {
                e.name = nameBuf;
                if (m_ctxRef->scenePanelRef)
                    m_ctxRef->scenePanelRef->MarkSceneAsDirty();
                break;
            }
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("ID: %u", (unsigned int)entity);
    ImGui::PopStyleVar();

    ImGui::Separator();
    ImGui::Spacing();

    ImGuiContext& g = *ImGui::GetCurrentContext();
    const bool editedBefore = g.ActiveIdHasBeenEditedThisFrame;

    // Draw each component section if the entity has that component
    if (Transform2D* t = m_ecsRef->GetComponent<Transform2D>(entity))
    {
        ImTextureID iconHandle = m_iconTransform ? (ImTextureID)(intptr_t)m_iconTransform->GetHandle() : 0;
        drawComponent("Transform2D", *t, iconHandle,
            [&]
            {
                m_ecsRef->RemoveComponent<Transform2D>(entity);
                if (m_ecsRef->HasComponent<WorldTransform2D>(entity))
                    m_ecsRef->RemoveComponent<WorldTransform2D>(entity);
            });
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

        std::string before = s->texture
            ? ToProjectRelative(m_contentLoaderRef->GetTexturePath(s->texture), projectRoot) : "";
        s->texturePath = before;

        auto picker = [this, entity, projectRoot](const char*, const char*,
            std::function<void(const std::string&)>)
            {
                std::string assetsDir = projectRoot.empty() ? "" : (projectRoot / "assets").string();
                std::string picked = OpenNativeFileDialog("Select Texture", "Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.tga)", "*.png;*.jpg;*.jpeg;*.bmp;*.tga", assetsDir);
                if (picked.empty()) return;

                Sprite2D* sp = m_ecsRef->GetComponent<Sprite2D>(entity);
                if (!sp) return;
                if (Texture* loaded = m_contentLoaderRef->LoadTexture(picked))
                {
                    sp->texture = loaded;
                    sp->texturePath = ToProjectRelative(picked, projectRoot);
                    if (m_ctxRef->scenePanelRef) m_ctxRef->scenePanelRef->MarkSceneAsDirty();
                }
            };

        drawComponent("Sprite2D", *s, iconHandle,
            [&] { m_ecsRef->RemoveComponent<Sprite2D>(entity); },
            picker);

        s = m_ecsRef->GetComponent<Sprite2D>(entity);
        if (s)
        {
            if (s->texturePath != before)
            {
                Texture* loaded = m_contentLoaderRef->LoadTexture(
                    ResolveProjectPath(s->texturePath, projectRoot).string());
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
            // Clear the flag first
            tm->ClearRebuildFlag();

            // Copy everything out
            std::string mapPath = tm->GetSourceMapPath();
            std::vector<TilesetSource2D> tilesets = tm->GetTilesetSources();
            int tileW = tm->GetTileWidth();
            int tileH = tm->GetTileHeight();
            int collisionLayer = tm->collisionLayer;
            std::string collisionGids = tm->collisionGids;
            std::string pendingTex = tm->pendingTexturePath;
            int pendingGid = tm->pendingFirstgid;

            if (!tilesets.empty() && !mapPath.empty())
            {
                Tilemap2D fresh(tileW, tileH);

                bool tilesetsOk = true;
                for (const auto& src : tilesets)
                {
                    const Texture* tex = m_contentLoaderRef->LoadTexture(src.texturePath);
                    if (!tex)
                    {
                        Debug::LogError("[Tilemap] Failed to load tileset: %s", src.texturePath.c_str());
                        tilesetsOk = false;
                        break;
                    }
                    fresh.AddTileset(tex, src.texturePath, src.firstgid);
                }

                if (tilesetsOk)
                {
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
        }
    }

    if (BehaviorScript* bs = m_ecsRef->GetComponent<BehaviorScript>(entity))
        drawBehaviorScript(*bs);

    ImGui::Spacing();
    drawAddComponentMenu();

    ImGuiWindow* inspectorWin = ImGui::GetCurrentWindow();
    if (ImGui::BeginDragDropTargetCustom(inspectorWin->Rect(), ImGui::GetID("##InspectorDrop")))
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetPayload))
        {
            std::filesystem::path dropped(static_cast<const char*>(payload->Data));
            std::string ext = dropped.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
            {
                if (!m_ecsRef->HasComponent<Sprite2D>(entity))
                    m_ecsRef->AddComponent<Sprite2D>(entity, Sprite2D());

                if (Texture* tex = m_contentLoaderRef->LoadTexture(dropped.string()))
                {
                    Sprite2D* sp = m_ecsRef->GetComponent<Sprite2D>(entity);
                    sp->texture = tex;
                    sp->texturePath = ToProjectRelative(dropped, projectRoot);
                }
            }
            else if (ext == ".hpp")
            {
                if (!m_ecsRef->HasComponent<BehaviorScript>(entity))
                    m_ecsRef->AddComponent<BehaviorScript>(entity, BehaviorScript());

                BehaviorScript* bs = m_ecsRef->GetComponent<BehaviorScript>(entity);
                std::filesystem::path noExt = dropped;
                noExt.replace_extension();
                bs->scriptPath = ToProjectRelative(noExt, projectRoot);
                bs->behaviorName = dropped.stem().string();
                if (m_ctxRef->scenePanelRef)
                    m_ctxRef->scenePanelRef->RequestScriptBuild();
            }
            else
            {
                Debug::LogWarn("[Inspector] No component accepts '%s' files", ext.c_str());
            }

            if (m_ctxRef->scenePanelRef)
                m_ctxRef->scenePanelRef->MarkSceneAsDirty();
        }
        ImGui::EndDragDropTarget();
    }

    if (m_pendingPicker.has_value())
    {
        ImGui::OpenPopup(m_pendingPicker->popupId);
        m_pickerOpen = true;
    }

    if (m_pickerOpen && m_pendingPicker.has_value())
    {
        auto& req = *m_pendingPicker;
        ProjectManager* pm = Services::Fetch<ProjectManager>();
        std::string path = req.initialPath;
        if (path.empty() && pm && pm->HasProject())
        {
            if (req.filterExt == nullptr || req.filterExt[0] == '\0')
                path = pm->Current().scripts.string();
            else
                path = pm->Current().root.string();
        }
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

    if (ImGui::Button("Add Component", ImVec2(-1.0f, 0.0f)))
        ImGui::OpenPopup("AddComponentPopup");

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        ImGui::TextDisabled("Components");
        ImGui::Separator();

        if (!m_ecsRef->HasComponent<Transform2D>(entity) && ImGui::MenuItem("Transform2D"))
        {
            m_ecsRef->AddComponent<Transform2D>(entity, Transform2D());
            if (!m_ecsRef->HasComponent<WorldTransform2D>(entity))
                m_ecsRef->AddComponent<WorldTransform2D>(entity, WorldTransform2D{});
        }

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
        static bool s_dirInitialized = false;

        // Pre fill the directory with the project's scripts folder on first use
        if (!s_dirInitialized)
        {
            ProjectManager* pm = Services::Fetch<ProjectManager>();
            if (pm && pm->HasProject())
            {
                std::string scriptsDir = pm->Current().scripts.string();
                std::strncpy(s_dirBuf, scriptsDir.c_str(), sizeof(s_dirBuf) - 1);
                s_dirBuf[sizeof(s_dirBuf) - 1] = '\0';
            }
            s_dirInitialized = true;
        }

        static std::vector<std::filesystem::path> s_scripts;
        static char s_scriptFilter[64] = {};

        if (ImGui::Button("Select Existing Script...", ImVec2(-1.0f, 0.0f)))
        {
            s_scripts.clear();
            s_scriptFilter[0] = '\0';
            ProjectManager* pm = Services::Fetch<ProjectManager>();
            if (pm && pm->HasProject())
            {
                std::error_code ec;
                std::filesystem::recursive_directory_iterator it(pm->Current().scripts, ec), end;
                for (; !ec && it != end; it.increment(ec))
                    if (it->is_regular_file(ec) && it->path().extension() == ".hpp")
                        s_scripts.push_back(it->path());
                std::sort(s_scripts.begin(), s_scripts.end());
            }
            ImGui::OpenPopup("##PickScript");
        }

        if (ImGui::BeginPopup("##PickScript"))
        {
            ImGui::SetNextItemWidth(260.0f);
            ImGui::InputTextWithHint("##scriptFilter", "Search...", s_scriptFilter, sizeof(s_scriptFilter));
            ImGui::Separator();
            ImGui::BeginChild("##scriptList", ImVec2(260.0f, 200.0f));
            for (const auto& p : s_scripts)
            {
                std::string name = p.stem().string();
                if (s_scriptFilter[0] && name.find(s_scriptFilter) == std::string::npos) continue;
                if (ImGui::Selectable(name.c_str()))
                {
                    ProjectManager* pm = Services::Fetch<ProjectManager>();
                    std::filesystem::path noExt = p;
                    noExt.replace_extension();
                    bs.scriptPath = ToProjectRelative(noExt, pm ? pm->Current().root : std::filesystem::path());
                    bs.behaviorName = name;
                    if (m_ctxRef->scenePanelRef)
                        m_ctxRef->scenePanelRef->RequestScriptBuild();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }

        BeginPropertyTable();

        // Directory row
        NextRow();
        ImGui::PushID("scriptdir");
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Directory");
        ImGui::TableSetColumnIndex(1);
        float btnH = ImGui::GetFrameHeight();
        ImGui::SetNextItemWidth(-btnH - ImGui::GetStyle().ItemSpacing.x);
        ImGui::InputText("##dir", s_dirBuf, sizeof(s_dirBuf));
        ImGui::SameLine();
        if (ImGui::Button("...##dir", ImVec2(btnH, btnH)))
        {
            ProjectManager* pm = Services::Fetch<ProjectManager>();
            std::string startPath;
            if (s_dirBuf[0] != '\0' && std::filesystem::is_directory(s_dirBuf))
                startPath = s_dirBuf;
            else if (pm && pm->HasProject())
                startPath = pm->Current().scripts.string();

            std::string chosen = OpenNativeFolderDialog("Select Script Directory", startPath);
            if (!chosen.empty())
            {
                std::strncpy(s_dirBuf, chosen.c_str(), sizeof(s_dirBuf) - 1);
                s_dirBuf[sizeof(s_dirBuf) - 1] = '\0';
            }
        }
        ImGui::PopID();

        // Class name row
        NextRow();
        ImGui::PushID("scriptname");
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Class Name");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##name", s_classNameBuf, sizeof(s_classNameBuf));
        ImGui::PopID();

        // Full path preview
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
            if (m_ctxRef->scenePanelRef)
                m_ctxRef->scenePanelRef->RequestScriptBuild();
            s_classNameBuf[0] = '\0';
            // Keep s_dirBuf so the next script uses the same folder
        }

        if (!canCreate) ImGui::EndDisabled();
    }
    else
    {
        // Script bound
        BeginPropertyTable();
        NextRow(); FieldReadOnly("Name", bs.behaviorName.c_str());
        NextRow(); FieldReadOnly("Path", (bs.scriptPath + ".hpp").c_str());
        EndPropertyTable();

        if (ImGui::Button("Clear", ImVec2(-1.0f, 0.0f)))
        {
            bs.scriptPath.clear();
            bs.behaviorName.clear();
            if (m_ctxRef->scenePanelRef)
                m_ctxRef->scenePanelRef->RequestScriptBuild();
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
        if (m_ctxRef->scenePanelRef)
            m_ctxRef->scenePanelRef->RequestScriptBuild();
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
