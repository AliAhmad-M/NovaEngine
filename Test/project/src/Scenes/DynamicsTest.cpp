#define _CRT_SECURE_NO_WARNINGS

#include <Test/Scenes/DynamicsTest.h>
#include <Core/Components.h>
#include <Platform/Window.h>
#include <Platform/InputMap.h>
#include <Runtime/EngineRuntime.h>
#include <Graphics/Components.h>
#include <Graphics/Renderer.h>
#include <Graphics/Font.h>
#include <Physics/Components.h>
#include <Physics/Systems.h>

static ContentLoader* s_contentLoader;

static ECS* s_ecs;
static Window* s_window;
static InputMap* s_inputMap;
static Renderer* s_renderer;
static Font* s_font;

static constexpr float BALL_RADIUS = 12.0f;
static constexpr float BOX_WIDTH   = 24.0f;
static constexpr float BOX_HEIGHT  = 24.0f;

static float s_spawnTimer = 0.0f;
static constexpr float SPAWN_COOLDOWN = 0.05f;
static const Vector2 SPAWN_ORIGIN = Vector2(100.0f, 100.0f);

void DynamicsTest::Ready()
{
    s_contentLoader = m_context->Get<ContentLoader>();
    s_ecs = m_context->Get<ECS>();
    s_window = m_context->Get<Window>();
    s_inputMap = m_context->Get<InputMap>();
    s_renderer = m_context->Get<Renderer>();
    s_font = s_contentLoader->LoadFont("project/assets/boldpixels.ttf", 24.0f);

    s_inputMap->Bind("spawn_circle", GLFW_MOUSE_BUTTON_RIGHT);
    s_inputMap->Bind("spawn_box", GLFW_MOUSE_BUTTON_LEFT);

    s_spawnTimer = 0.0f;

    createStaticBodies();
}

void DynamicsTest::Update(float dt)
{
    // FPS counter
    m_fpsTimer += dt;
    m_frameCount++;
    if (m_fpsTimer >= 0.5f) 
    {
        m_currentFPS = (float)m_frameCount / m_fpsTimer;
        m_fpsTimer = 0.0f;
        m_frameCount = 0;
    }

    int windowWidth, windowHeight;
    s_window->GetSize(&windowWidth, &windowHeight);

    // Advance the internal continuous spawning clock safely
    if (s_spawnTimer > 0.0f) 
    {
        s_spawnTimer -= dt;
    }

    // Continuous fire check with added interval restrictions
    if (s_spawnTimer <= 0.0f) {
        if (s_inputMap->IsButtonDown("spawn_circle")) 
        {
            spawnCircle();
            s_spawnTimer = SPAWN_COOLDOWN; // Reset the clock sequence
        }
        else if (s_inputMap->IsButtonDown("spawn_box")) 
        {
            spawnBox();
            s_spawnTimer = SPAWN_COOLDOWN; // Reset the clock sequence
        }
    }

    s_ecs->GetSystem<RigidBodySystem2D>()->Update(dt);

    Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
        0, (float)windowWidth, (float)windowHeight, 0, -100.0f, 100.0f);

    s_renderer->Begin(projection);

    // Draw static bodies
    for (EntityID id : m_staticBodies) 
    {
        auto* trans = s_ecs->GetComponent<Transform2D>(id);
        auto* rb = s_ecs->GetComponent<RigidBody2D>(id);
        if (trans && rb) 
        {
            Vector2 size = Vector2(rb->GetHalfWidth() * 2.0f, rb->GetHalfHeight() * 2.0f);
            Vector2 topLeftPos = trans->position - Vector2(rb->GetHalfWidth(), rb->GetHalfHeight());
            s_renderer->DrawRect(topLeftPos, size, trans->rotation, Color::WHITE, true);
        }
    }

    // Draw circles
    for (EntityID id : m_spawnedBalls) 
    {
        auto* trans = s_ecs->GetComponent<Transform2D>(id);
        auto* rb = s_ecs->GetComponent<RigidBody2D>(id);
        if (trans && rb) 
            s_renderer->DrawCircle(trans->position, rb->GetRadius(), Color::CYAN, true);
    }

    // Draw boxes
    for (EntityID id : m_spawnedBoxes) 
    {
        auto* trans = s_ecs->GetComponent<Transform2D>(id);
        auto* rb = s_ecs->GetComponent<RigidBody2D>(id);
        if (trans && rb) 
        {
            Vector2 size = Vector2(rb->GetHalfWidth() * 2.0f, rb->GetHalfHeight() * 2.0f);
            Vector2 topLeftPos = trans->position - Vector2(rb->GetHalfWidth(), rb->GetHalfHeight());
            s_renderer->DrawRect(topLeftPos, size, trans->rotation, Color::GOLD, true);
        }
    }

    // HUD
    size_t total = m_spawnedBalls.size() + m_spawnedBoxes.size();
    s_renderer->DrawText(*s_font, "Physics Test Sandbox (Box2D)", Vector2(20.0f, 40.0f), 1.0f, Color::WHITE);
    char buf[64];
    sprintf(buf, "Entities: %zu | FPS: %.1f", total, m_currentFPS);
    s_renderer->DrawText(*s_font, buf, Vector2((float)windowWidth - 280.0f, 40.0f), 1.0f, Color::WHITE);

    s_renderer->Flush();
}

void DynamicsTest::Leave()
{
    s_ecs->DeleteAllEntities();
    m_spawnedBalls.clear();
    m_spawnedBoxes.clear();
    m_staticBodies.clear();
}

void DynamicsTest::spawnCircle()
{
    int mx, my;
    s_inputMap->GetLocalCursorPosition(&mx, &my);
    Vector2 mousePos((float)mx, (float)my);

    Vector2 launchVelocity = (mousePos - SPAWN_ORIGIN);

    EntityID id = s_ecs->CreateEntity();
    s_ecs->AddComponent<Transform2D>(id, Transform2D(SPAWN_ORIGIN, 0.0f, Vector2(1.0f, 1.0f)));
    s_ecs->AddComponent<RigidBody2D>(id, RigidBody2D(Vector2::ZERO, BALL_RADIUS, false, false, 1.0f, 0.5f, 0.3f));

    auto* liveRb = s_ecs->GetComponent<RigidBody2D>(id);
    if (liveRb) liveRb->SetLinearVelocity(launchVelocity);
    
    m_spawnedBalls.push_back(id);
}

void DynamicsTest::spawnBox()
{
    int mx, my;
    s_inputMap->GetLocalCursorPosition(&mx, &my);
    Vector2 mousePos((float)mx, (float)my);

    Vector2 launchVelocity = (mousePos - SPAWN_ORIGIN);

    EntityID id = s_ecs->CreateEntity();
    s_ecs->AddComponent<Transform2D>(id, Transform2D(SPAWN_ORIGIN, 0.0f, Vector2(1.0f, 1.0f)));
    s_ecs->AddComponent<RigidBody2D>(id, RigidBody2D(Vector2::ZERO, BOX_WIDTH, BOX_HEIGHT, false, false, 1.0f, 0.3f, 0.5f));

    auto* liveRb = s_ecs->GetComponent<RigidBody2D>(id);
    if (liveRb) liveRb->SetLinearVelocity(launchVelocity);

    m_spawnedBoxes.push_back(id);
}

void DynamicsTest::createStaticBodies()
{
    int windowWidth, windowHeight;
    s_window->GetSize(&windowWidth, &windowHeight);

    float fWidth = (float)windowWidth;
    float fHeight = (float)windowHeight;
    float thickness = 100.0f;
    float halfThick = thickness * 0.5f;

    // Top Wall
    Vector2 pos(fWidth * 0.5f, -halfThick);
    EntityID id = s_ecs->CreateEntity();
    s_ecs->AddComponent<Transform2D>(id, Transform2D(pos, 0.0f, Vector2(1.0f, 1.0f)));
    s_ecs->AddComponent<RigidBody2D>(id, RigidBody2D(Vector2::ZERO, fWidth, thickness, true, false, 1.0f, 0.5f, 0.3f));
    m_staticBodies.push_back(id);

    // Bottom Wall
    Vector2 pos(fWidth * 0.5f, fHeight + halfThick);
    EntityID id = s_ecs->CreateEntity();
    s_ecs->AddComponent<Transform2D>(id, Transform2D(pos, 0.0f, Vector2(1.0f, 1.0f)));
    s_ecs->AddComponent<RigidBody2D>(id, RigidBody2D(Vector2::ZERO, fWidth, thickness, true, false, 1.0f, 0.5f, 0.3f));
    m_staticBodies.push_back(id);

    // Left Wall
    Vector2 pos(-halfThick, fHeight * 0.5f);
    EntityID id = s_ecs->CreateEntity();
    s_ecs->AddComponent<Transform2D>(id, Transform2D(pos, 0.0f, Vector2(1.0f, 1.0f)));
    s_ecs->AddComponent<RigidBody2D>(id, RigidBody2D(Vector2::ZERO, thickness, fHeight, true, false, 1.0f, 0.5f, 0.3f));
    m_staticBodies.push_back(id);

    // Right Wall
    Vector2 pos(fWidth + halfThick, fHeight * 0.5f);
    EntityID id = s_ecs->CreateEntity();
    s_ecs->AddComponent<Transform2D>(id, Transform2D(pos, 0.0f, Vector2(1.0f, 1.0f)));
    s_ecs->AddComponent<RigidBody2D>(id, RigidBody2D(Vector2::ZERO, thickness, fHeight, true, false, 1.0f, 0.5f, 0.3f));
    m_staticBodies.push_back(id);
}