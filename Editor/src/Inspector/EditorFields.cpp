#include <Editor/Inspector/EditorFields.h>
#include <imgui.h>
#include <cmath>
#include <cstring>

using namespace Nova::Editor;

void EditorFields::BeginRow(const char* name)
{
    ImGui::TableNextRow();
    ImGui::PushID(name);
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(name);
    ImGui::TableSetColumnIndex(1);
}

void EditorFields::EndRow()
{
    ImGui::PopID();
}

void EditorFields::Header(const char* label)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextDisabled("%s", label);
    ImGui::TableSetColumnIndex(1);
}

void EditorFields::ReadOnly(const char* name, const char* value)
{
    BeginRow(name);
    ImGui::TextUnformatted(value);
    EndRow();
}

bool EditorFields::DrawFloatWidget(float& v, float speed, float vmin, float vmax,
    bool allowInfinity, float infinitySign)
{
    bool changed = false;

    if (allowInfinity)
    {
        bool isInf = std::isinf(v);
        float shown = isInf ? 0.0f : v;

        ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
        if (isInf) ImGui::BeginDisabled();
        if (ImGui::DragFloat("##v", &shown, speed))
        {
            v = shown;
            changed = true;
        }
        if (isInf) ImGui::EndDisabled();

        ImGui::SameLine();
        bool toggled = isInf;
        if (ImGui::Checkbox("\xE2\x88\x9E##inf", &toggled))
        {
            v = toggled ? infinitySign * INFINITY : 0.0f;
            changed = true;
        }
    }
    else if (vmin != 0.0f || vmax != 0.0f)
    {
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::SliderFloat("##v", &v, vmin, vmax))
            changed = true;
    }
    else
    {
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::DragFloat("##v", &v, speed))
            changed = true;
    }

    return changed;
}

void EditorFields::Float(const char* name, float* value,
    float speed, float vmin, float vmax, bool allowInfinity, float infinitySign)
{
    BeginRow(name);
    DrawFloatWidget(*value, speed, vmin, vmax, allowInfinity, infinitySign);
    EndRow();
}

void EditorFields::Float(const char* name, std::function<float()> get, std::function<void(float)> set,
    float speed, float vmin, float vmax, bool allowInfinity, float infinitySign)
{
    BeginRow(name);
    float v = get();
    if (DrawFloatWidget(v, speed, vmin, vmax, allowInfinity, infinitySign))
        set(v);
    EndRow();
}

bool EditorFields::DrawIntWidget(int& v, float speed, int vmin, int vmax)
{
    if (vmin != 0 || vmax != 0)
    {
        ImGui::SetNextItemWidth(-1.0f);
        return ImGui::SliderInt("##v", &v, vmin, vmax);
    }

    ImGui::SetNextItemWidth(-1.0f);
    return ImGui::DragInt("##v", &v, speed);
}

void EditorFields::Int(const char* name, int* value, float speed, int vmin, int vmax)
{
    BeginRow(name);
    DrawIntWidget(*value, speed, vmin, vmax);
    EndRow();
}

void EditorFields::Int(const char* name, std::function<int()> get, std::function<void(int)> set,
    float speed, int vmin, int vmax)
{
    BeginRow(name);
    int v = get();
    if (DrawIntWidget(v, speed, vmin, vmax))
        set(v);
    EndRow();
}

bool EditorFields::DrawVec2Widget(Vector2& v, float speed)
{
    ImGui::SetNextItemWidth(-1.0f);
    return ImGui::DragFloat2("##v", &v.x, speed);
}

void EditorFields::Vec2(const char* name, Vector2* value, float speed)
{
    BeginRow(name);
    DrawVec2Widget(*value, speed);
    EndRow();
}

void EditorFields::Vec2(const char* name, std::function<Vector2()> get, std::function<void(Vector2)> set,
    float speed)
{
    BeginRow(name);
    Vector2 v = get();
    if (DrawVec2Widget(v, speed))
        set(v);
    EndRow();
}

bool EditorFields::DrawBoolWidget(bool& v)
{
    return ImGui::Checkbox("##v", &v);
}

void EditorFields::Bool(const char* name, bool* value)
{
    BeginRow(name);
    DrawBoolWidget(*value);
    EndRow();
}

void EditorFields::Bool(const char* name, std::function<bool()> get, std::function<void(bool)> set)
{
    BeginRow(name);
    bool v = get();
    if (DrawBoolWidget(v))
        set(v);
    EndRow();
}

bool EditorFields::DrawStringWidget(std::string& v)
{
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s", v.c_str());
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##v", buf, sizeof(buf)))
    {
        v = buf;
        return true;
    }
    return false;
}

void EditorFields::String(const char* name, std::string* value)
{
    BeginRow(name);
    DrawStringWidget(*value);
    EndRow();
}

void EditorFields::String(const char* name, std::function<std::string()> get, std::function<void(std::string)> set)
{
    BeginRow(name);
    std::string v = get();
    if (DrawStringWidget(v))
        set(v);
    EndRow();
}