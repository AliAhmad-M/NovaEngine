#include <Editor/Panels/InspectorPanel.h>
#include <imgui.h>

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

static bool BeginComponent(const char* name, bool* pOpen = nullptr)
{
    ImGui::PushID(name);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_AllowOverlap |
        ImGuiTreeNodeFlags_FramePadding;

    bool open = ImGui::TreeNodeEx(name, flags);

    // Remove-component button
    if (pOpen)
    {
        float btnW = ImGui::GetFrameHeight();
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - btnW + ImGui::GetCursorPosX()
            - ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("x", ImVec2(btnW, btnW))) *pOpen = false;
    }

    // Clean up ID stack
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

// Two-column property table helpers
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

InspectorPanel::InspectorPanel(EditorContext* ctx) : m_ctx(ctx) {}

void InspectorPanel::Draw()
{
    ImGui::Begin("Inspector");

    if (m_ctx->selectedEntity == -1)
    {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    // Entity name header (stub)
    char nameLabel[64];
    std::snprintf(nameLabel, sizeof(nameLabel), "Entity %d", m_ctx->selectedEntity);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::Text("%s", nameLabel);
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::Spacing();

    // Draw each component block
    if (m_hasTransform)   DrawTransform2D();
    if (m_hasSprite)      DrawSprite2D();
    if (m_hasCollider)    DrawCollider2D();
    if (m_hasPhysics)     DrawPhysics2D();
    if (m_hasMonoBehavior) DrawMonoBehavior();

    ImGui::Spacing();
    DrawAddComponentMenu();

    ImGui::End();
}

void InspectorPanel::DrawTransform2D()
{
    if (!BeginComponent("Transform2D", &m_hasTransform)) return;

    BeginPropertyTable();

    NextRow(); FieldVec2("Position", m_pos);
    NextRow(); FieldFloat("Rotation", &m_rot, 0.5f);
    NextRow(); FieldVec2("Scale", m_scale);

    EndPropertyTable();
    EndComponent();
}

void InspectorPanel::DrawSprite2D()
{
    if (!BeginComponent("Sprite2D", &m_hasSprite)) return;

    BeginPropertyTable();

    NextRow(); FieldString("Texture", m_texturePath, sizeof(m_texturePath));

    // Inline "Browse" button on the same row
    ImGui::SameLine();
    if (ImGui::SmallButton("..."))
    {
        // TODO: open file picker
    }

    NextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Color");
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::ColorEdit4("##color", m_color);

    EndPropertyTable();
    EndComponent();
}

void InspectorPanel::DrawCollider2D()
{
    if (!BeginComponent("Collider2D", &m_hasCollider)) return;

    BeginPropertyTable();

    NextRow(); FieldVec2("Offset", m_colliderOffset);
    NextRow(); FieldVec2("Size", m_colliderSize, 0.5f);
    NextRow(); FieldBool("Is Trigger", &m_isTrigger);

    EndPropertyTable();
    EndComponent();
}

void InspectorPanel::DrawPhysics2D()
{
    if (!BeginComponent("Physics2D", &m_hasPhysics)) return;

    BeginPropertyTable();

    NextRow(); FieldFloat("Mass", &m_mass, 0.01f, 0.001f, 1000.0f);
    NextRow(); FieldFloat("Restitution", &m_restitution, 0.01f, 0.0f, 1.0f);
    NextRow(); FieldBool("Is Kinematic", &m_isKinematic);
    NextRow(); FieldVec2("Velocity", m_velocity);

    EndPropertyTable();
    EndComponent();
}

void InspectorPanel::DrawMonoBehavior()
{
    if (!BeginComponent("MonoBehavior", &m_hasMonoBehavior)) return;

    BeginPropertyTable();
    NextRow(); FieldString("Script", m_scriptName, sizeof(m_scriptName));
    EndPropertyTable();

    ImGui::Spacing();
    ImGui::TextDisabled("Custom behavior properties will appear here.");

    EndComponent();
}

void InspectorPanel::DrawAddComponentMenu()
{
    // "Add Component" button
    float btnW = 200.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - btnW) * 0.5f);
    if (ImGui::Button("Add Component", ImVec2(btnW, 0)))
        ImGui::OpenPopup("AddComponentPopup");

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        ImGui::TextDisabled("Components");
        ImGui::Separator();

        if (!m_hasTransform && ImGui::MenuItem("Transform2D"))   m_hasTransform = true;
        if (!m_hasSprite && ImGui::MenuItem("Sprite2D"))      m_hasSprite = true;
        if (!m_hasCollider && ImGui::MenuItem("Collider2D"))    m_hasCollider = true;
        if (!m_hasPhysics && ImGui::MenuItem("Physics2D"))     m_hasPhysics = true;
        if (!m_hasMonoBehavior && ImGui::MenuItem("MonoBehavior")) m_hasMonoBehavior = true;

        ImGui::EndPopup();
    }
}

