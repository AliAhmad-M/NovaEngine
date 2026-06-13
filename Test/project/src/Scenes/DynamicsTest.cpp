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
#include <Runtime/EngineRuntime.h>

#include <glad/glad.h> 

static ContentLoader* s_contentLoader;

static ECS*      s_ecs;
static Window*   s_window;
static InputMap* s_inputMap;
static Renderer* s_renderer;
static Font*     s_font;

// Cooldown timer between continuous spawns
static float s_spawnCooldown    = 0.0f;
static const float SPAWN_DELAY  = 0.05f;

// Static definition of one meter in pixels
static const float METER        = 32.0f;

void DynamicsTest::Ready()
{
    s_contentLoader = m_context->Get<ContentLoader>();
    
    s_ecs      = m_context->Get<ECS>();
    s_window   = m_context->Get<Window>();
    s_inputMap = m_context->Get<InputMap>();
    s_renderer = m_context->Get<Renderer>();
    s_font     = s_contentLoader->LoadFont("project/assets/boldpixels.ttf", 24.0f);

    // Bind inputs
    s_inputMap->Bind("spawn_continuous", GLFW_MOUSE_BUTTON_LEFT);
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
    s_window->GetSize(&windowWidth, &windowHeight);

    // Continuous spawn handled with the timer gate
    if (s_inputMap->IsButtonDown("spawn_continuous") && s_spawnCooldown <= 0.0f) {
        spawnBallWithImpulse();
        s_spawnCooldown = SPAWN_DELAY;
    }

    // 360Hz physics substeps
    float substepDelta = dt / 6.0f;
    for (int substep = 0; substep < 6; substep++)
    {
        // Accumulate forces
        for (EntityID ballId : m_spawnedBalls) {
            auto* ballPhysics = s_ecs->GetComponent<Physics2D>(ballId);
            if (ballPhysics) {
                ballPhysics->ApplyForce(Vector2(0.0f, GLOBAL_GRAVITY * ballPhysics->GetMass()));
            }
        }

        // Collisions
        s_ecs->GetSystem<CollisionSystem2D>()->Update(substepDelta);
        
        // Physics
        s_ecs->GetSystem<PhysicsSystem2D>()->Update(substepDelta);

        // Constraint ball position to window
        applyWindowConstraints(windowWidth, windowHeight);
    }

    Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(0, (float)windowWidth, (float)windowHeight, 0, -100.0f, 100.0f);

    // Rendering
    s_renderer->Begin(projection);
        // Draw balls
        for (EntityID ballId : m_spawnedBalls) {
            // Render projectiles
            auto* trans = s_ecs->GetComponent<Transform2D>(ballId);
            auto* col = s_ecs->GetComponent<Collider2D>(ballId);
            if (trans && col) {
                Vector2 center = col->GetGlobalCenter(trans);
                float radius = col->GetRadius(trans->scale.x, trans->scale.y);
                s_renderer->DrawCircle(center, radius, Color::CYAN, true);
            }
        }

        // Draw text
        s_renderer->DrawText(*s_font, "Physics Test Sandbox", Vector2(20.0f, 40.0f), 1.0f, Color::WHITE);
        char statsBuffer[64]; sprintf(statsBuffer, "Entities: %zu | FPS: %.1f", m_spawnedBalls.size(), m_currentFPS);
        s_renderer->DrawText(*s_font, statsBuffer, Vector2((float)windowWidth - 280.0f, 40.0f), 1.0f, Color::WHITE);
    s_renderer->Flush();
}

void DynamicsTest::Leave()
{
    // Clear scene
    s_ecs->DeleteAllEntities();
    m_spawnedBalls.clear();
}

void DynamicsTest::spawnBallWithImpulse()
{
    int mouseX, mouseY;
    s_inputMap->GetLocalCursorPosition(&mouseX, &mouseY);

    // Calculate launch direction based on mouse coordinates
    Vector2 cursorCoords = Vector2((float)mouseX, (float)mouseY);
    Vector2 launchDirection = cursorCoords - m_spawnPoint;
    float distance = Vector2::Magnitude(launchDirection);

    if (distance > 0.0f) {
        // Compute launch velocity
        Vector2 normalizedDir = Vector2::Normalize(launchDirection);
        Vector2 initialVelocity = normalizedDir * distance;

        // Spawn the ball entity
        EntityID ball = s_ecs->CreateEntity();
        s_ecs->AddComponent<Transform2D>(ball, Transform2D(m_spawnPoint, 0.0f, Vector2(1.0f, 1.0f)));
        s_ecs->AddComponent<Collider2D>(ball, Collider2D(Vector2(BALL_RADIUS, BALL_RADIUS), BALL_RADIUS, true));
        s_ecs->AddComponent<Physics2D>(ball, Physics2D(1.2f, false, 0.2f, 0.7f));

        s_ecs->GetComponent<Physics2D>(ball)->linearVelocity = initialVelocity;
        m_spawnedBalls.push_back(ball);
    }
}

void DynamicsTest::applyWindowConstraints(int windowWidth, int windowHeight)
{
    // Make sure all balls stay within window
    for (EntityID ballId : m_spawnedBalls) {
        auto* trans = s_ecs->GetComponent<Transform2D>(ballId);
        auto* col = s_ecs->GetComponent<Collider2D>(ballId);
        auto* phys = s_ecs->GetComponent<Physics2D>(ballId);

        if (trans && col && phys) {
            float radius = col->GetRadius(trans->scale.x, trans->scale.y);

            // Left
            if (trans->position.x - radius < 0.0f) {
                trans->position.x = radius;
                if (phys->linearVelocity.x < 0.0f) phys->linearVelocity.x = -phys->linearVelocity.x * phys->restitution;
            }

            // Right
            else if (trans->position.x + radius > (float)windowWidth) {
                trans->position.x = (float)windowWidth - radius;
                if (phys->linearVelocity.x > 0.0f) phys->linearVelocity.x = -phys->linearVelocity.x * phys->restitution;
            }

            // Bottom
            if (trans->position.y + radius > (float)windowHeight) {
                trans->position.y = (float)windowHeight - radius;
                if (phys->linearVelocity.y > 0.0f) phys->linearVelocity.y = -phys->linearVelocity.y * phys->restitution;
            }

            // Top
            else if (trans->position.y - radius < 0.0f) {
                trans->position.y = radius;
                if (phys->linearVelocity.y < 0.0f) phys->linearVelocity.y = -phys->linearVelocity.y * phys->restitution;
            }
        }
    }
}