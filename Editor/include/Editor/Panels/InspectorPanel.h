#pragma once

#include <Editor/EditorContext.h>
#include <imgui.h>

class InspectorPanel
{
public:
    explicit InspectorPanel(EditorContext* ctx);

    void Draw();
    void SetWindowFlags(ImGuiWindowFlags flags) { m_windowFlags = flags; }

private:
    void DrawTransform2D();
    void DrawSprite2D();
	void DrawRigidBody2D();
    void DrawCustomBehavior();
    void DrawAddComponentMenu();

    EditorContext* m_ctx = nullptr;
    ImGuiWindowFlags m_windowFlags = 0;

    bool m_hasTransform = true;
    bool m_hasSprite2D = false;
    bool m_hasRigidBody2D = false;
    bool m_hasCustomBehavior = false;

    // Placeholder transform data
    float m_pos[2] = { 0.0f, 0.0f };
    float m_rot = 0.0f;
    float m_scale[2] = { 1.0f, 1.0f };

    // Placeholder sprite data
    char  m_texturePath[256] = {};
    float m_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Placeholder collider data
    float m_colliderOffset[2] = { 0.0f, 0.0f };
    float m_colliderSize[2] = { 32.0f, 32.0f };
    bool  m_isTrigger = false;

    // Placeholder physics data
    float m_density = 1.0f;
    float m_restitution = 0.3f;
    float m_linVelocity[2] = { 0.0f, 0.0f };

    // Placeholder custom behaviour
    char m_scriptName[128] = "MyBehavior";
};
