#pragma once
#include <Core/ECS/ECS.h>
#include <Graphics/Texture.h>
#include <Runtime/Scene/SceneManager.h>

class GraphicsTest : public Nova::IScene
{
public:
    void Ready() override;
    void Render() override;
    void Update(float dt) override;
    void Leave() override;

private:
    Nova::Texture * m_ninjaTexture = nullptr;
    Nova::EntityID m_ninjaEntity  = 0;
    Nova::EntityID m_cameraEntity = 0;

    std::string m_lastDirection = "idle_down";

    float m_fpsTimer = 0.0f;
    float m_currentFPS = 0.0f;
    int m_frameCount = 0;
};