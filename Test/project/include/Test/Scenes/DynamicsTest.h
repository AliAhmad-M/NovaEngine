#pragma once

#include <Core/EngineContext.h>
#include <Core/ECS/ECS.h>
#include <Core/Utilities/Vector2.h>
#include <Runtime/Scene/SceneManager.h>

#include <vector>

class DynamicsTest : public Scene
{
public:
    DynamicsTest(EngineContext* context) : Scene(context) {}

    void Ready() override;
    void Update(float dt) override;
    void Leave() override;

private:
    std::vector<EntityID> m_spawnedBalls;
    Vector2 m_spawnPoint = Vector2(100.0f, 100.0f);
    const float BALL_RADIUS = 8.0f;
    float m_currentFPS = 0.0f;
    float m_fpsTimer = 0.0f;
    int m_frameCount = 0;

    void spawnBallWithImpulse();
    void applyWindowConstraints(int windowWidth, int windowHeight);
};