#pragma once
#include <Core/ECS/ECS.h>
#include <Core/Utilities/Vector2.h>
#include <Runtime/Scene/SceneManager.h>

#include <vector>

class DynamicsTest : public IScene
{
public:
    void Ready() override;
    void Update(float dt) override;
    void Leave() override;

private:
    std::vector<EntityID> m_spawnedBalls;
    std::vector<EntityID> m_staticBodies;

    float m_currentFPS = 0.0f;
    float m_fpsTimer = 0.0f;
    int   m_frameCount = 0;

    void spawnCircle();
    void createStaticBodies();
};