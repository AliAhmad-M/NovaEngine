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
    std::vector<EntityID> m_spawnedBoxes;
    std::vector<EntityID> m_staticBodies;

    float m_currentFPS = 0.0f;
    float m_fpsTimer = 0.0f;
    int   m_frameCount = 0;

    void spawnCircle();
    void spawnBox();
    void createStaticBodies();
};