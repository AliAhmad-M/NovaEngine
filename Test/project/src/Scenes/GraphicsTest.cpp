#define _CRT_SECURE_NO_WARNINGS
#include <Test/Scenes/GraphicsTest.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Graphics/Systems.h>
#include <Graphics/Renderer.h>
#include <Graphics/Font.h>
#include <Graphics/Texture/TextureLoader.h>
#include <Physics/Components.h>
#include <Physics/Systems.h>
#include <Platform/Window.h>
#include <Platform/InputMap.h>

extern Window        g_window;
extern Renderer      g_renderer;
extern InputMap      g_inputMap;
extern TextureLoader g_textureLoader;
extern Font          g_engineFont;
extern ECS           g_ecs;

void GraphicsTest::Ready()
{
    // Load texture
    m_ninjaTexture = g_textureLoader.LoadTexture("project/assets/ninja.png");

    // Create entity
    m_ninjaEntity = g_ecs.CreateEntity();
    g_ecs.AddComponent<Transform2D>(m_ninjaEntity, Transform2D(Vector2(640.0f, 360.0f), 0.0f, Vector2(4.0f, 4.0f)));

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
    g_ecs.AddComponent<Sprite2D>(m_ninjaEntity, sprite);

    // Add collider component
    Vector2 centerOffset = Vector2(8.0f, 14.0f);
    float colliderWidth = 10.0f;
    float colliderHeight = 6.0f;
    g_ecs.AddComponent<Collider2D>(m_ninjaEntity, Collider2D(centerOffset, colliderWidth, colliderHeight));

    // Bind inputs
    g_inputMap.Bind("move_up", GLFW_KEY_UP);
    g_inputMap.Bind("move_down", GLFW_KEY_DOWN);
    g_inputMap.Bind("move_left", GLFW_KEY_LEFT);
    g_inputMap.Bind("move_right", GLFW_KEY_RIGHT);

    // Initialize tilemap
    int tileSize = 16;
    float tileScaling = 4.0f;

    Tilemap tilemap = Tilemap(&g_ecs, tileSize, tileSize, Vector2(tileScaling, tileScaling));

    tilemap.AddTileset(g_textureLoader.LoadTexture("project/assets/spring.png"), 1);
    tilemap.AddTileset(g_textureLoader.LoadTexture("project/assets/dirt.png"), 29);
    tilemap.AddTileset(g_textureLoader.LoadTexture("project/assets/tree.png"), 44);
    tilemap.AddTileset(g_textureLoader.LoadTexture("project/assets/water - spring.png"), 76);
    tilemap.AddTileset(g_textureLoader.LoadTexture("project/assets/texture.png"), 91);
    tilemap.AddTileset(g_textureLoader.LoadTexture("project/assets/summer.png"), 123);

    tilemap.LoadTilemap("project/assets/tilemap.json");
    tilemap.GenerateMap(&g_renderer);

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

    auto* transform = g_ecs.GetComponent<Transform2D>(m_ninjaEntity);
    auto* sprite = g_ecs.GetComponent<Sprite2D>(m_ninjaEntity);

    float moveSpeed = 150.0f;
    bool  isMoving = false;

    // Track last facing direction
    std::string targetAnimation = m_lastDirection;

    // Movement + walk animations
    if (g_inputMap.IsButtonDown("move_up")) {
        transform->position.y -= moveSpeed * dt;
        targetAnimation = "walk_up";
        m_lastDirection = "idle_up";
        isMoving = true;
    }
    else if (g_inputMap.IsButtonDown("move_down")) {
        transform->position.y += moveSpeed * dt;
        targetAnimation = "walk_down";
        m_lastDirection = "idle_down";
        isMoving = true;
    }

    if (g_inputMap.IsButtonDown("move_left")) {
        transform->position.x -= moveSpeed * dt;
        targetAnimation = "walk_left";
        m_lastDirection = "idle_left";
        isMoving = true;
    }
    else if (g_inputMap.IsButtonDown("move_right")) {
        transform->position.x += moveSpeed * dt;
        targetAnimation = "walk_right";
        m_lastDirection = "idle_right";
        isMoving = true;
    }

    // Switch to directional idle when no key held
    if (!isMoving) targetAnimation = m_lastDirection;

    sprite->SetAnimation(targetAnimation);

    int windowWidth, windowHeight;
    g_window.GetSize(&windowWidth, &windowHeight);
    Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
        0, (float)windowWidth, (float)windowHeight, 0, -100.0f, 100.0f);

    g_renderer.Begin(projection);

    // Update systems
    g_ecs.GetSystem<SpriteSystem>()->Update(dt);
    g_ecs.GetSystem<CollisionSystem2D>()->Update(dt);

    // Draw UI
    g_renderer.DrawText(g_engineFont, "Graphics Animation Test", Vector2(20.0f, 40.0f), 1.0f, Color::WHITE);

    char statsBuffer[64];
    sprintf(statsBuffer, "Entities: %zu | FPS: %.1f", g_ecs.GetEntityCount(), m_currentFPS);
    g_renderer.DrawText(g_engineFont, statsBuffer, Vector2((float)windowWidth - 280.0f, 40.0f), 1.0f, Color::WHITE);

    g_renderer.Flush();
}

void GraphicsTest::Leave()
{
    // Clear entities
    g_ecs.DeleteAllEntities();
}