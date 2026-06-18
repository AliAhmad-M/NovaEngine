#pragma once

#include <Editor/EditorContext.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Physics/Components.h>
#include <imgui.h>

class InspectorPanel
{
public:
    explicit InspectorPanel(EditorContext* ctx);

    void Draw();
    void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

private:
    void DrawTransform2D(Transform2D& t);
    void DrawSprite2D(Sprite2D& s);
    void DrawRigidBody2D(RigidBody2D& rb);
    void DrawAddComponentMenu();

    void DrawTexturePathField(Sprite2D& s);

    EditorContext* m_ctx = nullptr;
    ImGuiWindowFlags m_windowFlags = 0;
};