#pragma once
#include <Runtime/Scene/IScene.h>
#include <Core/ECS/ECS.h>
#include <Graphics/Texture/Texture.h>

class GraphicsTest : public IScene
{
public:
    void Ready() override;
    void Update(float dt) override;
    void Leave() override;

private:
    Texture* m_ninjaTexture = nullptr;
    EntityID m_ninjaEntity;

    float m_fpsTimer = 0.0f;
    float m_currentFPS = 0.0f;
    int m_frameCount = 0;

    std::string m_lastDirection = "idle_down";
};