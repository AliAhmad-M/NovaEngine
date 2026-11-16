#include <Editor/Inspector/EditorFields.h>
#include <Editor/FilePicker.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>
#include <cstdio>
#include <algorithm>

using namespace Nova::Editor;

void EditorFields::pushFieldStyle()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.12f, 0.14f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.16f, 0.19f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.20f, 0.24f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.26f, 0.30f, 0.65f));
}

void EditorFields::popFieldStyle()
{
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);
}

bool EditorFields::drawToggleSwitch(const char* id, bool& v)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const float height = ImGui::GetFrameHeight() * 0.82f;
    const float width = height * 1.75f;
    const float radius = height * 0.5f;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    pos.y += (ImGui::GetFrameHeight() - height) * 0.5f;

    ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
    ImGui::ItemSize(ImVec2(width, ImGui::GetFrameHeight()));
    if (!ImGui::ItemAdd(bb, window->GetID(id)))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, window->GetID(id), &hovered, &held);
    if (pressed)
        v = !v;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImU32 bgCol = v
        ? ImGui::GetColorU32(hovered ? ImVec4(0.28f, 0.58f, 1.00f, 1.00f) : ImVec4(0.22f, 0.52f, 0.96f, 1.00f))
        : ImGui::GetColorU32(hovered ? ImVec4(0.24f, 0.26f, 0.30f, 1.00f) : ImVec4(0.18f, 0.19f, 0.22f, 1.00f));

    draw->AddRectFilled(bb.Min, bb.Max, bgCol, radius);
    draw->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(0.24f, 0.26f, 0.30f, 0.65f)), radius);

    float knobRadius = radius - 2.5f;
    float knobX = v ? (bb.Max.x - radius) : (bb.Min.x + radius);
    draw->AddCircleFilled(ImVec2(knobX, bb.Min.y + radius), knobRadius, IM_COL32(245, 247, 250, 255));

    return pressed;
}

bool EditorFields::drawAxisFloat(const char* axisLabel, const char* dragId, float& val,
    float speed, float width, const ImVec4& baseCol, const ImVec4& hoverCol)
{
    bool changed = false;
    float h = ImGui::GetFrameHeight();
    float capW = 20.0f;
    float capGap = 3.0f;
    float dragW = std::max(32.0f, width - capW - capGap);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, ImGui::GetStyle().FramePadding.y));
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_Button, baseCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, baseCol);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.95f));

    // Reset axis value on cap click
    if (ImGui::Button(axisLabel, ImVec2(capW, h)))
    {
        val = 0.0f;
        changed = true;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Click to reset to 0");

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);

    ImGui::SameLine(0.0f, capGap);
    ImGui::SetNextItemWidth(dragW);
    if (ImGui::DragFloat(dragId, &val, speed, 0.0f, 0.0f, "%.2f"))
        changed = true;

    return changed;
}

void EditorFields::beginRow(const char* name)
{
    ImGui::TableNextRow();
    ImGui::PushID(name);

    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.76f, 0.78f, 0.82f, 1.0f));
    ImGui::TextUnformatted(name);
    ImGui::PopStyleColor();

    ImGui::TableSetColumnIndex(1);
}

void EditorFields::endRow()
{
    ImGui::PopID();
}

void EditorFields::Header(const char* label)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.62f, 0.72f, 1.0f));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::TableSetColumnIndex(1);
}

void EditorFields::ReadOnly(const char* name, const char* value)
{
    beginRow(name);
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.58f, 0.64f, 1.0f));
    ImGui::TextUnformatted(value);
    ImGui::PopStyleColor();
    endRow();
}

bool EditorFields::drawFloatWidget(float& v, float speed, float vmin, float vmax,
    bool allowInfinity, float infinitySign)
{
    pushFieldStyle();
    bool changed = false;

    if (allowInfinity)
    {
        bool isInf = std::isinf(v);
        float shown = isInf ? 0.0f : v;
        float btnW = 28.0f;

        ImGui::SetNextItemWidth(-(btnW + 4.0f));
        if (isInf)
            ImGui::BeginDisabled();
        if (ImGui::DragFloat("##v", &shown, speed, 0.0f, 0.0f, isInf ? "Infinity" : "%.2f"))
        {
            v = shown;
            changed = true;
        }
        if (isInf)
            ImGui::EndDisabled();

        ImGui::SameLine(0.0f, 4.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, isInf ? ImVec4(0.22f, 0.52f, 0.96f, 1.00f) : ImVec4(0.11f, 0.12f, 0.14f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, isInf ? ImVec4(0.28f, 0.58f, 1.00f, 1.00f) : ImVec4(0.15f, 0.16f, 0.19f, 1.00f));
        if (ImGui::Button("\xE2\x88\x9E##inf", ImVec2(btnW, ImGui::GetFrameHeight())))
        {
            v = !isInf ? (infinitySign * INFINITY) : 0.0f;
            changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Toggle Infinity");
        ImGui::PopStyleColor(2);
    }
    else if (vmin != 0.0f || vmax != 0.0f)
    {
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.22f, 0.52f, 0.96f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.35f, 0.65f, 1.00f, 1.00f));
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::SliderFloat("##v", &v, vmin, vmax, "%.2f"))
            changed = true;
        ImGui::PopStyleColor(2);
    }
    else
    {
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::DragFloat("##v", &v, speed, 0.0f, 0.0f, "%.2f"))
            changed = true;
    }

    popFieldStyle();
    return changed;
}

void EditorFields::Float(const char* name, float* value,
    float speed, float vmin, float vmax, bool allowInfinity, float infinitySign)
{
    beginRow(name);
    drawFloatWidget(*value, speed, vmin, vmax, allowInfinity, infinitySign);
    endRow();
}

void EditorFields::Float(const char* name, std::function<float()> get, std::function<void(float)> set,
    float speed, float vmin, float vmax, bool allowInfinity, float infinitySign)
{
    beginRow(name);
    float v = get();
    if (drawFloatWidget(v, speed, vmin, vmax, allowInfinity, infinitySign))
        set(v);
    endRow();
}

bool EditorFields::drawIntWidget(int& v, float speed, int vmin, int vmax)
{
    pushFieldStyle();
    bool changed = false;

    if (vmin != 0 || vmax != 0)
    {
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.22f, 0.52f, 0.96f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.35f, 0.65f, 1.00f, 1.00f));
        ImGui::SetNextItemWidth(-1.0f);
        changed = ImGui::SliderInt("##v", &v, vmin, vmax);
        ImGui::PopStyleColor(2);
    }
    else
    {
        ImGui::SetNextItemWidth(-1.0f);
        changed = ImGui::DragInt("##v", &v, speed);
    }

    popFieldStyle();
    return changed;
}

void EditorFields::Int(const char* name, int* value, float speed, int vmin, int vmax)
{
    beginRow(name);
    drawIntWidget(*value, speed, vmin, vmax);
    endRow();
}

void EditorFields::Int(const char* name, std::function<int()> get, std::function<void(int)> set,
    float speed, int vmin, int vmax)
{
    beginRow(name);
    int v = get();
    if (drawIntWidget(v, speed, vmin, vmax))
        set(v);
    endRow();
}

bool EditorFields::drawVec2Widget(Vector2& v, float speed)
{
    pushFieldStyle();
    bool changed = false;

    const float gap = 6.0f;
    float availW = ImGui::GetContentRegionAvail().x;
    float halfW = (availW - gap) * 0.5f;

    changed |= drawAxisFloat("X##vec2", "##x", v.x, speed, halfW,
        ImVec4(0.82f, 0.24f, 0.24f, 1.00f), ImVec4(0.92f, 0.32f, 0.32f, 1.00f));
    ImGui::SameLine(0.0f, gap);
    changed |= drawAxisFloat("Y##vec2", "##y", v.y, speed, halfW,
        ImVec4(0.24f, 0.72f, 0.32f, 1.00f), ImVec4(0.32f, 0.82f, 0.40f, 1.00f));

    popFieldStyle();
    return changed;
}

void EditorFields::Vec2(const char* name, Vector2* value, float speed)
{
    beginRow(name);
    drawVec2Widget(*value, speed);
    endRow();
}

void EditorFields::Vec2(const char* name, std::function<Vector2()> get, std::function<void(Vector2)> set,
    float speed)
{
    beginRow(name);
    Vector2 v = get();
    if (drawVec2Widget(v, speed))
        set(v);
    endRow();
}

bool EditorFields::drawBoolWidget(bool& v)
{
    return drawToggleSwitch("##v", v);
}

void EditorFields::Bool(const char* name, bool* value)
{
    beginRow(name);
    drawBoolWidget(*value);
    endRow();
}

void EditorFields::Bool(const char* name, std::function<bool()> get, std::function<void(bool)> set)
{
    beginRow(name);
    bool v = get();
    if (drawBoolWidget(v))
        set(v);
    endRow();
}

bool EditorFields::drawStringWidget(std::string& v)
{
    pushFieldStyle();
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s", v.c_str());

    ImGui::SetNextItemWidth(-1.0f);
    bool changed = false;
    if (ImGui::InputText("##v", buf, sizeof(buf)))
    {
        v = buf;
        changed = true;
    }

    popFieldStyle();
    return changed;
}

void EditorFields::String(const char* name, std::string* value)
{
    beginRow(name);
    drawStringWidget(*value);
    endRow();
}

void EditorFields::String(const char* name, std::function<std::string()> get, std::function<void(std::string)> set)
{
    beginRow(name);
    std::string v = get();
    if (drawStringWidget(v))
        set(v);
    endRow();
}

void EditorFields::Path(const char* label, std::string* path, const char* filterExt)
{
    pushFieldStyle();
    beginRow(label);

    float btnW = 28.0f;
    ImGui::SetNextItemWidth(-(btnW + 4.0f));

    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s", path->c_str());
    if (ImGui::InputTextWithHint("##v", "None (Asset Path)", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue))
        *path = buf;

    ImGui::SameLine(0.0f, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.11f, 0.12f, 0.14f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.16f, 0.19f, 1.00f));
    if (ImGui::Button("...##browse", ImVec2(btnW, ImGui::GetFrameHeight())) && requestPicker)
    {
        requestPicker("##pickerPath", filterExt,
            [path](const std::string& picked) { *path = picked; });
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Browse asset");
    ImGui::PopStyleColor(2);

    endRow();
    popFieldStyle();
}

void EditorFields::Group(const char* label, std::function<void()> body)
{
    Header(label);
    body();
}

void EditorFields::Button(const char* label, std::function<void()> onClick)
{
    ImGui::TableNextRow();
    ImGui::PushID(label);
    ImGui::TableSetColumnIndex(1);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.20f, 0.25f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.52f, 0.96f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.42f, 0.85f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.26f, 0.30f, 0.65f));

    if (ImGui::Button(label, ImVec2(-1.0f, ImGui::GetFrameHeight() + 2.0f)) && onClick)
        onClick();

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
}