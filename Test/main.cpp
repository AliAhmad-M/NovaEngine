#include <thread>
#include <cstdio>
#include <vector>
#include <Core/ECS/ECS.h>
#include <Core/Components.h>
#include <Platform/Window.h>
#include <Platform/InputMap.h>
#include <Graphics/Components.h>
#include <Graphics/Camera/Camera2D.h>
#include <Graphics/Renderer.h>
#include <Graphics/Texture/TextureLoader.h>
#include <Graphics/Systems.h>
#include <Physics/Systems.h>
#include <Physics/Components/Physics2D.h>

Window        g_window;
Renderer      g_renderer;
InputMap      g_inputMap;
TextureLoader g_textureLoader;
bool          g_running = true;
double        g_deltaTime = 0;

const float SLIME_FORCE = 256.0f;
const float SLIME_MAX_VEL = 64.0f;
const float SLIME_DAMPING = 0.82f;

// Obstacles
const float SLIME_RADIUS = 32.0f;

int main() {
    if (!g_window.Init(1280, 720, "Engine Test"))
        return -1;

    g_renderer.Init();

    ECS ecs;
    Camera2D camera;

    ecs.Init();

    // Register Components
    ecs.RegisterComponent<Transform2D>();
    ecs.RegisterComponent<Sprite2D>();
    ecs.RegisterComponent<Collider2D>();
    ecs.RegisterComponent<Physics2D>();

    // Register Systems
    ecs.RegisterSystem<SpriteSystem>(&ecs, &camera, &g_window, &g_renderer);
    ecs.AddComponentToSystem<Transform2D, SpriteSystem>();
    ecs.AddComponentToSystem<Sprite2D, SpriteSystem>();

    ecs.RegisterSystem<CollisionSystem2D>(&ecs);
    ecs.AddComponentToSystem<Transform2D, CollisionSystem2D>();
    ecs.AddComponentToSystem<Collider2D, CollisionSystem2D>();

    ecs.RegisterSystem<PhysicsSystem2D>(&ecs);
    ecs.AddComponentToSystem<Transform2D, PhysicsSystem2D>();
    ecs.AddComponentToSystem<Physics2D, PhysicsSystem2D>();

    // Input
    g_inputMap.Bind("move_l", GLFW_KEY_LEFT);
    g_inputMap.Bind("move_d", GLFW_KEY_DOWN);
    g_inputMap.Bind("move_r", GLFW_KEY_RIGHT);
    g_inputMap.Bind("move_u", GLFW_KEY_UP);

    Texture* slimeTex = g_textureLoader.LoadTexture("assets/slime.png");

    // Player
    EntityID slime = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(slime, Transform2D({ 600.0f, 320.0f }, 0.0f, { 4.0f, 4.0f }));
    ecs.AddComponent<Collider2D>(slime, Collider2D({ 8.0f, 8.0f }, 8.0f));
    ecs.AddComponent<Physics2D>(slime, Physics2D(1.0f, false));

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

    // Box Obstacles
    EntityID box1 = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(box1, Transform2D({ 200.0f, 150.0f }, 0.0f, { 1.0f, 1.0f }));
    ecs.AddComponent<Collider2D>(box1, Collider2D({ 32.0f, 32.0f }, 64.0f, 64.0f, true, true));

    EntityID box2 = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(box2, Transform2D({ 950.0f, 500.0f }, 0.0f, { 1.0f, 1.0f }));
    ecs.AddComponent<Collider2D>(box2, Collider2D({ 32.0f, 32.0f }, 64.0f, 64.0f, true, true));

    EntityID box3 = ecs.CreateEntity();
    ecs.AddComponent<Transform2D>(box3, Transform2D({ 500.0f, 550.0f }, 0.0f, { 1.0f, 1.0f }));
    ecs.AddComponent<Collider2D>(box3, Collider2D({ 32.0f, 32.0f }, 64.0f, 64.0f, true, true));

    // Circle Obstacles
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

    while (g_running) {
        double currentTime = glfwGetTime();
        g_deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        g_window.PollEvents();
        if (g_window.ShouldClose()) g_running = false;
        g_inputMap.Update(g_window.GetNativeWindow());

        // Input
        auto* transform = ecs.GetComponent<Transform2D>(slime);
        auto* rb = ecs.GetComponent<Physics2D>(slime);
        auto* sp = ecs.GetComponent<Sprite2D>(slime);

        float dx = 0.0f, dy = 0.0f;

        if (g_inputMap.IsButtonDown("move_l")) dx -= 1.0f;
        if (g_inputMap.IsButtonDown("move_r")) dx += 1.0f;
        if (g_inputMap.IsButtonDown("move_u")) dy -= 1.0f;
        if (g_inputMap.IsButtonDown("move_d")) dy += 1.0f;

        bool hasInput = dx != 0.0f || dy != 0.0f;

        if (hasInput)
        {
            Vector2 moveDir = Vector2::Normalize(Vector2(dx, dy));
            rb->ApplyForce(moveDir * SLIME_FORCE);
        }
        else
        {
            // Damp velocity when no input so slime doesn't slide forever
            rb->velocity = rb->velocity * SLIME_DAMPING;
        }

        // Clamp velocity to max
        float speed = Vector2::Magnitude(rb->velocity);
        if (speed > SLIME_MAX_VEL)
            rb->velocity = Vector2::Normalize(rb->velocity) * SLIME_MAX_VEL;

        // Animation
        if (dx < 0.0f) { sp->SetAnimation("walk_left");  lastIdleAnim = "idle_left"; }
        else if (dx > 0.0f) { sp->SetAnimation("walk_right"); lastIdleAnim = "idle_right"; }
        else if (dy < 0.0f) { sp->SetAnimation("walk_up");    lastIdleAnim = "idle_up"; }
        else if (dy > 0.0f) { sp->SetAnimation("walk_down");  lastIdleAnim = "idle_down"; }
        else { sp->SetAnimation(lastIdleAnim); }

        // Physics
        ecs.GetSystem<PhysicsSystem2D>()->Update((float)g_deltaTime);
        ecs.GetSystem<CollisionSystem2D>()->Update((float)g_deltaTime);

        // Render
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int windowWidth, windowHeight;
        g_window.GetSize(&windowWidth, &windowHeight);

        Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
            0, (float)windowWidth, (float)windowHeight, 0, -1.0f, 1.0f
        );

        g_renderer.Begin(projection);

        ecs.GetSystem<SpriteSystem>()->Update();

        // Draw obstacle colliders
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

        // Draw slime collider
        auto* slimeTransform = ecs.GetComponent<Transform2D>(slime);
        auto* slimeCollider = ecs.GetComponent<Collider2D>(slime);

        Vector2 slimeCenterPoint = slimeCollider->GetGlobalCenter(slimeTransform);
        float   slimeRadius = slimeCollider->GetRadius(slimeTransform->scale.x, slimeTransform->scale.y);

        g_renderer.DrawCircle(slimeCenterPoint, slimeRadius, Color::GREEN, true);
        g_renderer.DrawCircle(slimeCenterPoint, 3.0f, Color::RED, false);
        g_renderer.DrawCircle(slimeTransform->position, 3.0f, Color::BLUE, false);

        g_renderer.Flush();
        g_window.SwapBuffers();

        while (glfwGetTime() < lastTime + targetFrameTime)
            std::this_thread::yield();
    }

    g_textureLoader.Cleanup();
    g_window.Exit();
    return 0;
}