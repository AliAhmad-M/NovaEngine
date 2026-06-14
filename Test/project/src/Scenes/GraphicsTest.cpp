#define _CRT_SECURE_NO_WARNINGS

#include <Test/Scenes/GraphicsTest.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Graphics/Systems.h>
#include <Graphics/Renderer.h>
#include <Graphics/Font.h>
#include <Runtime/ContentLoader.h>
#include <Physics/Components.h>
#include <Physics/Systems.h>
#include <Platform/Window.h>
#include <Platform/InputMap.h>
#include <Runtime/EngineRuntime.h>

#include <glad/glad.h> 

static ContentLoader* s_contentLoader;

static ECS* s_ecs;
static Window* s_window;
static InputMap* s_inputMap;
static Renderer* s_renderer;
static Font* s_font;
static Camera2D* s_camera2D;

void GraphicsTest::Ready()
{
    // Load engine context
    s_contentLoader = m_context->Get<ContentLoader>();
    s_ecs = m_context->Get<ECS>();
    s_window = m_context->Get<Window>();
    s_inputMap = m_context->Get<InputMap>();
    s_renderer = m_context->Get<Renderer>();
    s_font = s_contentLoader->LoadFont("project/assets/boldpixels.ttf", 24.0f);
    s_camera2D = m_context->Get<Camera2D>();

    // Update camera bounds
    s_camera2D->SetBoundaries(s_window, 0, 1280, 0, 960);

    // Load texture
    m_ninjaTexture = s_contentLoader->LoadTexture("project/assets/ninja.png");

    // Create entity
    m_ninjaEntity = s_ecs->CreateEntity();
    s_ecs->AddComponent<Transform2D>(m_ninjaEntity, Transform2D(Vector2(640.0f, 360.0f), 0.0f, Vector2(4.0f, 4.0f)));

    // 4x7 spritesheet, one column per direction
    Sprite2D sprite(m_ninjaTexture, 4, 7, 2);
    sprite.AddAnimation("idle_down", { 0 }, 0);
    sprite.AddAnimation("idle_up", { 1 }, 0);
    sprite.AddAnimation("idle_left", { 2 }, 0);
    sprite.AddAnimation("idle_right", { 3 }, 0);
    sprite.AddAnimation("walk_down", { 0,  4,  8, 12 }, 8);
    sprite.AddAnimation("walk_up", { 1,  5,  9, 13 }, 8);
    sprite.AddAnimation("walk_left", { 2,  6, 10, 14 }, 8);
    sprite.AddAnimation("walk_right", { 3,  7, 11, 15 }, 8);

    sprite.SetAnimation("idle_down");
    s_ecs->AddComponent<Sprite2D>(m_ninjaEntity, sprite);

    // Add collider component
    Vector2 centerOffset = Vector2(8.0f, 14.0f);
    float colliderWidth = 10.0f;
    float colliderHeight = 6.0f;
    s_ecs->AddComponent<Collider2D>(m_ninjaEntity, Collider2D(centerOffset, colliderWidth, colliderHeight));

    // Bind inputs
    s_inputMap->Bind("move_up", GLFW_KEY_UP);
    s_inputMap->Bind("move_down", GLFW_KEY_DOWN);
    s_inputMap->Bind("move_left", GLFW_KEY_LEFT);
    s_inputMap->Bind("move_right", GLFW_KEY_RIGHT);

    // Initialize tilemap
    int tileSize = 16;
    float tileScaling = 4.0f;

    Tilemap tilemap = Tilemap(s_ecs, tileSize, tileSize, Vector2(tileScaling, tileScaling));

    tilemap.AddTileset(s_contentLoader->LoadTexture("project/assets/spring.png"), 1);
    tilemap.AddTileset(s_contentLoader->LoadTexture("project/assets/dirt.png"), 29);
    tilemap.AddTileset(s_contentLoader->LoadTexture("project/assets/tree.png"), 44);
    tilemap.AddTileset(s_contentLoader->LoadTexture("project/assets/water - spring.png"), 76);
    tilemap.AddTileset(s_contentLoader->LoadTexture("project/assets/texture.png"), 91);
    tilemap.AddTileset(s_contentLoader->LoadTexture("project/assets/summer.png"), 123);

    tilemap.LoadTilemap("project/assets/tilemap.json");
    tilemap.GenerateMap(s_renderer);

    tilemap.AddCollision(1, { 59, 60, 61, 62, 63, 64, 65, 66 });
}

void GraphicsTest::Update(float dt)
{
    // FPS
    m_fpsTimer += dt;
    m_frameCount++;
    if (m_fpsTimer >= 0.5f) {
        m_currentFPS = (float)m_frameCount / m_fpsTimer;
        m_fpsTimer = 0.0f;
        m_frameCount = 0;
    }

    auto* transform = s_ecs->GetComponent<Transform2D>(m_ninjaEntity);
    auto* sprite = s_ecs->GetComponent<Sprite2D>(m_ninjaEntity);

    float moveSpeed = 150.0f;
    bool  isMoving = false;

    int windowWidth, windowHeight;
    s_window->GetSize(&windowWidth, &windowHeight);

    // Update camera offset
    s_camera2D->SetOffset(s_window, 
        transform->position - Vector2((float)windowWidth * 0.5f, (float)windowHeight * 0.5f));

    // Track last facing direction
    std::string targetAnimation = m_lastDirection;

    // Movement + walk animations
    if (s_inputMap->IsButtonDown("move_up")) {
        transform->position.y -= moveSpeed * dt;
        targetAnimation = "walk_up";
        m_lastDirection = "idle_up";
        isMoving = true;
    }
    else if (s_inputMap->IsButtonDown("move_down")) {
        transform->position.y += moveSpeed * dt;
        targetAnimation = "walk_down";
        m_lastDirection = "idle_down";
        isMoving = true;
    }

    if (s_inputMap->IsButtonDown("move_left")) {
        transform->position.x -= moveSpeed * dt;
        targetAnimation = "walk_left";
        m_lastDirection = "idle_left";
        isMoving = true;
    }
    else if (s_inputMap->IsButtonDown("move_right")) {
        transform->position.x += moveSpeed * dt;
        targetAnimation = "walk_right";
        m_lastDirection = "idle_right";
        isMoving = true;
    }

    // Switch to directional idle when no key held
    if (!isMoving) targetAnimation = m_lastDirection;

    sprite->SetAnimation(targetAnimation);

    Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
        0, (float)windowWidth, (float)windowHeight, 0, -100.0f, 100.0f);

    // Collisions
    s_ecs->GetSystem<CollisionSystem2D>()->Update(dt);
    
    // Rendering
    s_renderer->Begin(projection);
        //Draw sprites
        s_ecs->GetSystem<SpriteSystem2D>()->Update(dt);

        // Draw text
        s_renderer->DrawText(*s_font, "Graphics Animation Test", Vector2(20.0f, 40.0f), 1.0f, Color::WHITE);        
        char statsBuffer[64]; sprintf(statsBuffer, "Entities: %zu | FPS: %.1f", s_ecs->GetEntityCount(), m_currentFPS);
        s_renderer->DrawText(*s_font, statsBuffer, Vector2((float)windowWidth - 280.0f, 40.0f), 1.0f, Color::WHITE);
    s_renderer->Flush();
}

void GraphicsTest::Leave()
{
    // Clear entities
    s_ecs->DeleteAllEntities();
}