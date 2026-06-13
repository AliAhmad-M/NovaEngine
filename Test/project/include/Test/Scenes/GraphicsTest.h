#pragma once
#include <Core/EngineContext.h>
#include <Core/ECS/ECS.h>
#include <Graphics/Texture.h>
#include <Runtime/Tilemap/Tilemap.h>
#include <Runtime/Scene/SceneManager.h>

class GraphicsTest : public Scene
{
public:
    GraphicsTest(EngineContext* context) : Scene(context) {}
    
    void Ready() override;
    void Update(float dt) override;
    void Leave() override;

private:
    Texture* m_ninjaTexture = nullptr;
    EntityID m_ninjaEntity  = 0;
    std::string m_lastDirection = "idle_down";

    float m_fpsTimer = 0.0f;
    float m_currentFPS = 0.0f;
    int m_frameCount = 0;
};