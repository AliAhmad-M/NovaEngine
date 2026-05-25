#include <thread>
#include <cstdio>
#include <Core/ECS/ECS.h>
#include <Core/Components.h>
#include <Platform/Window.h>
#include <Platform/InputMap.h>
#include <Graphics/Components.h>
#include <Graphics/Systems/SpriteSystem.h>
#include <Graphics/Camera/Camera2D.h>
#include <Graphics/Renderer.h>
#include <Graphics/Texture/TextureLoader.h>
#include <Physics/Systems/CollisionSystem.h>

Window        g_window;
Renderer      g_renderer;
InputMap      g_inputMap;
TextureLoader g_textureLoader;
bool          g_running = true;
double        g_deltaTime = 0;

const float SLIME_SPEED = 100.0f;

int main() {
    if (!g_window.Init(1280, 720, "Slime Test"))
        return -1;

    g_renderer.Init();

    ECS ecs;
    Camera2D camera;

    ecs.Init();

    // --- Register Components ---
    ecs.RegisterComponent<Transform2D>();
    ecs.RegisterComponent<Sprite2D>();
    ecs.RegisterComponent<Collider2D>();

    // --- Register Systems ---
    ecs.RegisterSystem<SpriteSystem>(&ecs, &camera, &g_window, &g_renderer);
    ecs.AddComponentToSystem<Transform2D, SpriteSystem>();
    ecs.AddComponentToSystem<Sprite2D, SpriteSystem>();

    ecs.RegisterSystem<CollisionSystem>(&ecs);
    ecs.AddComponentToSystem<Transform2D, CollisionSystem>();
    ecs.AddComponentToSystem<Collider2D, CollisionSystem>();

    // --- Input Bindings ---
    g_inputMap.Bind("move_l", GLFW_KEY_LEFT);
    g_inputMap.Bind("move_d", GLFW_KEY_DOWN);
    g_inputMap.Bind("move_r", GLFW_KEY_RIGHT);
    g_inputMap.Bind("move_u", GLFW_KEY_UP);

    Texture* slimeTex = g_textureLoader.LoadTexture("assets/slime.png");

    // --- Player: circle collider (controlled) ---
    // Scale 4x4, sprite is 16x16 px -> rendered at 64x64, radius 8 * 4 = 32px
    EntityID slime = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(slime, Transform2D({ 600.0f, 320.0f }, 0.0f, { 4.0f, 4.0f }));
    ecs.AddComponent<Collider2D>(slime, Collider2D({ 8.0f, 8.0f }, 8.0f));

    Sprite2D sprite(slimeTex, 4, 4, 0);
    sprite.AddAnimation("idle_down", { 0 }, 1);
    sprite.AddAnimation("idle_up", { 1 }, 1);
    sprite.AddAnimation("idle_left", { 2 }, 1);
    sprite.AddAnimation("idle_right", { 3 }, 1);
    sprite.AddAnimation("walk_down", { 0, 4, 8,  12 }, 8);
    sprite.AddAnimation("walk_up", { 1, 5, 9,  13 }, 8);
    sprite.AddAnimation("walk_left", { 2, 6, 10, 14 }, 8);
    sprite.AddAnimation("walk_right", { 3, 7, 11, 15 }, 8);
    sprite.SetAnimation("idle_down");
    ecs.AddComponent<Sprite2D>(slime, sprite);

    // Slime rendered size: 64x64, radius ~32px
    // Obstacles kept roughly the same size (scale 1, local size 64x64 or radius 32)
    // Screen: 1280x720 — all positions chosen so colliders stay within bounds

    // --- Box Obstacles ---
    // Box: local 64x64, scale 1 -> world 64x64, centered at position + half extents
    EntityID box1 = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(box1, Transform2D({ 200.0f, 150.0f }, 0.0f, { 1.0f, 1.0f }));
    ecs.AddComponent<Collider2D>(box1, Collider2D({ 32.0f, 32.0f }, 64.0f, 64.0f, true, true));

    EntityID box2 = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(box2, Transform2D({ 950.0f, 500.0f }, 0.0f, { 1.0f, 1.0f }));
    ecs.AddComponent<Collider2D>(box2, Collider2D({ 32.0f, 32.0f }, 64.0f, 64.0f, true, true));

    EntityID box3 = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(box3, Transform2D({ 500.0f, 550.0f }, 0.0f, { 1.0f, 1.0f }));
    ecs.AddComponent<Collider2D>(box3, Collider2D({ 32.0f, 32.0f }, 64.0f, 64.0f, true, true));

    // --- Circle Obstacles ---
    // Circle: local offset 32,32, radius 32 -> world radius 32px
    EntityID circle1 = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(circle1, Transform2D({ 850.0f, 150.0f }, 0.0f, { 1.0f, 1.0f }));
    ecs.AddComponent<Collider2D>(circle1, Collider2D({ 32.0f, 32.0f }, 32.0f, true));

    EntityID circle2 = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(circle2, Transform2D({ 300.0f, 500.0f }, 0.0f, { 1.0f, 1.0f }));
    ecs.AddComponent<Collider2D>(circle2, Collider2D({ 32.0f, 32.0f }, 32.0f, true));

    EntityID circle3 = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(circle3, Transform2D({ 1100.0f, 350.0f }, 0.0f, { 1.0f, 1.0f }));
    ecs.AddComponent<Collider2D>(circle3, Collider2D({ 32.0f, 32.0f }, 32.0f, true));

    struct ObstacleInfo { EntityID id; bool isCircle; };
    std::vector<ObstacleInfo> obstacles = {
        { box1,    false },
        { box2,    false },
        { box3,    false },
        { circle1, true  },
        { circle2, true  },
        { circle3, true  },
    };

    const char* lastIdleAnim = "idle_down";
    const double targetFrameTime = 1.0 / 60.0;
    double lastTime = glfwGetTime();

    float printTimer = 0.0f;

    while (g_running) {
        double currentTime = glfwGetTime();
        g_deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        g_window.PollEvents();
        if (g_window.ShouldClose()) g_running = false;
        g_inputMap.Update(g_window.GetNativeWindow());

        // --- Input ---
        Transform2D* transform = ecs.GetComponent<Transform2D>(slime);
        Sprite2D* sp = ecs.GetComponent<Sprite2D>(slime);

        float dx = 0.0f, dy = 0.0f;

        if (g_inputMap.IsButtonDown("move_l")) dx -= 1.0f;
        if (g_inputMap.IsButtonDown("move_r")) dx += 1.0f;
        if (g_inputMap.IsButtonDown("move_u")) dy -= 1.0f;
        if (g_inputMap.IsButtonDown("move_d")) dy += 1.0f;

        Vector2 moveDirection = Vector2::Normalize(Vector2(dx, dy));

        transform->position.x += moveDirection.x * SLIME_SPEED * (float)g_deltaTime;
        transform->position.y += moveDirection.y * SLIME_SPEED * (float)g_deltaTime;

        // --- Animation (horizontal takes priority) ---
        if (dx < 0.0f) { sp->SetAnimation("walk_left");  lastIdleAnim = "idle_left"; }
        else if (dx > 0.0f) { sp->SetAnimation("walk_right"); lastIdleAnim = "idle_right"; }
        else if (dy < 0.0f) { sp->SetAnimation("walk_up");    lastIdleAnim = "idle_up"; }
        else if (dy > 0.0f) { sp->SetAnimation("walk_down");  lastIdleAnim = "idle_down"; }
        else { sp->SetAnimation(lastIdleAnim); }

        // --- Physics ---
        ecs.GetSystem<CollisionSystem>()->Update((float)g_deltaTime);

        // --- Logging ---
        auto* slimeTransform = ecs.GetComponent<Transform2D>(slime);
        auto* slimeCollider = ecs.GetComponent<Collider2D>(slime);

        printTimer += (float)g_deltaTime;
        if (printTimer >= 0.5f)
        {
            printTimer = 0.0f;
        }

        // --- Render ---
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int windowWidth, windowHeight;
        g_window.GetSize(&windowWidth, &windowHeight);

        Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
            0, (float)windowWidth, (float)windowHeight, 0, -1.0f, 1.0f
        );

        g_renderer.Begin(projection);

        // --- Sprites ---
        ecs.GetSystem<SpriteSystem>()->Update();

        // --- Debug: Draw Obstacle Colliders ---
        for (auto& obs : obstacles)
        {
            auto* obsTransform = ecs.GetComponent<Transform2D>(obs.id);
            auto* obsCollider = ecs.GetComponent<Collider2D>(obs.id);

            if (obs.isCircle)
            {
                Vector2 center = obsCollider->GetGlobalCenter(obsTransform);
                float radius = obsCollider->GetRadius(obsTransform->scale.x, obsTransform->scale.y);
                g_renderer.DrawCircle(center, radius, Color::WHITE, true);
                g_renderer.DrawCircle(center, 3.0f, Color::MAGENTA, false);
            }
            else
            {
                Rectangle* aabb = obsCollider->GetAABB(obsTransform);
                float minX, minY, maxX, maxY;
                aabb->GetMin(minX, minY);
                aabb->GetMax(maxX, maxY);
                g_renderer.DrawRect({ minX, minY }, { maxX - minX, maxY - minY }, 0.0f, Color::WHITE, true);

                Vector2 center = obsCollider->GetGlobalCenter(obsTransform);
                g_renderer.DrawCircle(center, 3.0f, Color::MAGENTA, false);
            }

            g_renderer.DrawCircle(obsTransform->position, 3.0f, Color::CYAN, false);
        }

        // --- Debug: Draw Slime ---
        Vector2 slimeCenterPoint = slimeCollider->GetGlobalCenter(slimeTransform);
        float slimeRadius = slimeCollider->GetRadius(slimeTransform->scale.x, slimeTransform->scale.y);

        g_renderer.DrawCircle(slimeCenterPoint, slimeRadius, Color::GREEN, true);
        g_renderer.DrawCircle(slimeCenterPoint, 3.0f, Color::RED, false);
        g_renderer.DrawCircle(slimeTransform->position, 3.0f, Color::BLUE, false);

        g_renderer.Flush();
        g_window.SwapBuffers();

        // --- Frame cap ---
        while (glfwGetTime() < lastTime + targetFrameTime)
            std::this_thread::yield();
    }

    g_textureLoader.Cleanup();
    g_window.Exit();
    return 0;
}