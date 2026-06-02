#pragma once
#include <Runtime/Scene/IScene.h>
#include <Core/ECS/ECS.h>
#include <Core/Utilities/Vector2.h>
#include <vector>

class DynamicsTest : public IScene
{
public:
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