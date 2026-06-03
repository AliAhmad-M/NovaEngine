#define _CRT_SECURE_NO_WARNINGS

#include <Test/Scenes/DynamicsTest.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Graphics/Renderer.h>
#include <Graphics/Font.h>
#include <Physics/Components.h>
#include <Physics/Systems.h>
#include <Platform/Window.h>
#include <Platform/InputMap.h>

#include <glad/glad.h> 

extern Window        g_window;
extern Renderer      g_renderer;
extern InputMap      g_inputMap;
extern Font          g_engineFont;
extern ECS           g_ecs;

#define METER 32

// Cooldown timer between continuous spawns
static float s_spawnCooldown = 0.0f;
static const float SPAWN_DELAY = 0.05f;

void DynamicsTest::Ready()
{
    // Bind inputs
    g_inputMap.Bind("spawn_continuous", GLFW_MOUSE_BUTTON_LEFT);
    s_spawnCooldown = 0.0f;
}

void DynamicsTest::Update(float dt)
{
    // Measure FPS
    m_fpsTimer += dt;
    m_frameCount++;
    if (m_fpsTimer >= 0.5f) {
        m_currentFPS = (float)m_frameCount / m_fpsTimer;
        m_fpsTimer = 0.0f;
        m_frameCount = 0;
    }

    // Tick down the continuous spawn timer
    if (s_spawnCooldown > 0.0f) {
        s_spawnCooldown -= dt;
    }

    const float GLOBAL_GRAVITY = 981.0f / METER;
    int windowWidth, windowHeight;
    g_window.GetSize(&windowWidth, &windowHeight);

    // Continuous spawn handled with the timer gate
    if (g_inputMap.IsButtonDown("spawn_continuous") && s_spawnCooldown <= 0.0f) {
        spawnBallWithImpulse();
        s_spawnCooldown = SPAWN_DELAY;
    }

    // 360Hz physics substeps
    float substepDelta = dt / 6.0f;
    for (int substep = 0; substep < 6; substep++)
    {
        // Accumulate forces
        for (EntityID ballId : m_spawnedBalls) {
            auto* ballPhysics = g_ecs.GetComponent<Physics2D>(ballId);
            if (ballPhysics) {
                ballPhysics->ApplyForce(Vector2(0.0f, GLOBAL_GRAVITY * ballPhysics->GetMass()));
            }
        }

        // Update systems
        g_ecs.GetSystem<CollisionSystem2D>()->Update(substepDelta);
        g_ecs.GetSystem<PhysicsSystem2D>()->Update(substepDelta);

        // Constraint ball position to window
        applyWindowConstraints(windowWidth, windowHeight);
    }

    Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(0, (float)windowWidth, (float)windowHeight, 0, -100.0f, 100.0f);

    g_renderer.Begin(projection);
    for (EntityID ballId : m_spawnedBalls) {
        // Render projectiles
        auto* trans = g_ecs.GetComponent<Transform2D>(ballId);
        auto* col = g_ecs.GetComponent<Collider2D>(ballId);
        if (trans && col) {
            Vector2 center = col->GetGlobalCenter(trans);
            float radius = col->GetRadius(trans->scale.x, trans->scale.y);
            g_renderer.DrawCircle(center, radius, Color::CYAN, true);
        }
    }
    g_renderer.Flush();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw UI
    g_renderer.Begin(projection);
    g_renderer.DrawText(g_engineFont, "Physics Test Sandbox", Vector2(20.0f, 40.0f), 1.0f, Color::WHITE);

    char statsBuffer[64];
    sprintf(statsBuffer, "Entities: %zu | FPS: %.1f", m_spawnedBalls.size(), m_currentFPS);
    g_renderer.DrawText(g_engineFont, statsBuffer, Vector2((float)windowWidth - 280.0f, 40.0f), 1.0f, Color::WHITE);
    g_renderer.Flush();

    // Restore state for the next frame
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void DynamicsTest::Leave()
{
    // Clear scene
    g_ecs.DeleteAllEntities();
    m_spawnedBalls.clear();
}

void DynamicsTest::spawnBallWithImpulse()
{
    int mouseX, mouseY;
    g_inputMap.GetLocalCursorPosition(&mouseX, &mouseY);

    // Calculate launch direction based on mouse coordinates
    Vector2 cursorCoords = Vector2((float)mouseX, (float)mouseY);
    Vector2 launchDirection = cursorCoords - m_spawnPoint;
    float distance = Vector2::Magnitude(launchDirection);

    if (distance > 0.0f) {
        // Compute launch velocity
        Vector2 normalizedDir = Vector2::Normalize(launchDirection);
        Vector2 initialVelocity = normalizedDir * distance;

        // Spawn the ball entity
        EntityID ball = g_ecs.CreateEntity();
        g_ecs.AddComponent<Transform2D>(ball, Transform2D(m_spawnPoint, 0.0f, Vector2(1.0f, 1.0f)));
        g_ecs.AddComponent<Collider2D>(ball, Collider2D(Vector2(BALL_RADIUS, BALL_RADIUS), BALL_RADIUS, true));
        g_ecs.AddComponent<Physics2D>(ball, Physics2D(1.2f, false, 0.2f, 0.7f));

        g_ecs.GetComponent<Physics2D>(ball)->velocity = initialVelocity;
        m_spawnedBalls.push_back(ball);
    }
}

void DynamicsTest::applyWindowConstraints(int windowWidth, int windowHeight)
{
    // Make sure all balls stay within window
    for (EntityID ballId : m_spawnedBalls) {
        auto* trans = g_ecs.GetComponent<Transform2D>(ballId);
        auto* col = g_ecs.GetComponent<Collider2D>(ballId);
        auto* phys = g_ecs.GetComponent<Physics2D>(ballId);

        if (trans && col && phys) {
            float radius = col->GetRadius(trans->scale.x, trans->scale.y);

            // Left
            if (trans->position.x - radius < 0.0f) {
                trans->position.x = radius;
                if (phys->velocity.x < 0.0f) phys->velocity.x = -phys->velocity.x * phys->restitution;
            }

            // Right
            else if (trans->position.x + radius > (float)windowWidth) {
                trans->position.x = (float)windowWidth - radius;
                if (phys->velocity.x > 0.0f) phys->velocity.x = -phys->velocity.x * phys->restitution;
            }

            // Bottom
            if (trans->position.y + radius > (float)windowHeight) {
                trans->position.y = (float)windowHeight - radius;
                if (phys->velocity.y > 0.0f) phys->velocity.y = -phys->velocity.y * phys->restitution;
            }

            // Top
            else if (trans->position.y - radius < 0.0f) {
                trans->position.y = radius;
                if (phys->velocity.y < 0.0f) phys->velocity.y = -phys->velocity.y * phys->restitution;
            }
        }
    }
}