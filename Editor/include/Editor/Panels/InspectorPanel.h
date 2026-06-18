#pragma once

#include <Editor/EditorContext.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Physics/Components.h>
#include <World/Components.h>
#include <imgui.h>

class InspectorPanel
{
public:
    explicit InspectorPanel(EditorContext* ctx);

    void Draw();
    void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

private:
    void drawTransform2D(Transform2D& t);
    void drawCamera2D(Camera2D& c);
    void drawSprite2D(Sprite2D& s);
    void drawRigidBody2D(RigidBody2D& rb);
    void drawTilemap2D(Tilemap2D& tm);

    void drawAddComponentMenu();

    void drawTexturePathField(Sprite2D& s);

    EditorContext* m_ctx = nullptr;
    ImGuiWindowFlags m_windowFlags = 0;
};