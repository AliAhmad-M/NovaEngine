#include <thread>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <algorithm>
#include <cmath>
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

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

Window        g_window;
Renderer      g_renderer;
InputMap      g_inputMap;
TextureLoader g_textureLoader;
bool          g_running = true;
double        g_deltaTime = 0;

#define METER 32

const float GLOBAL_GRAVITY = 981.0f / METER;
const float BALL_RADIUS = 8.0f;

int main()
{
    if (!g_window.Init(1280, 720, "Engine Test - Jolt Constrained Sandbox"))
        return -1;

    g_renderer.Init();

    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    ECS ecs;
    Camera2D camera;
    ecs.Init();

    ecs.RegisterComponent<Transform2D>();
    ecs.RegisterComponent<Sprite2D>();
    ecs.RegisterComponent<Collider2D>();
    ecs.RegisterComponent<Physics2D>();

    ecs.RegisterSystem<SpriteSystem>(&ecs, &camera, &g_window, &g_renderer);
    ecs.AddComponentToSystem<Transform2D, SpriteSystem>();
    ecs.AddComponentToSystem<Sprite2D, SpriteSystem>();

    ecs.RegisterSystem<CollisionSystem2D>(&ecs);
    ecs.AddComponentToSystem<Transform2D, CollisionSystem2D>();
    ecs.AddComponentToSystem<Collider2D, CollisionSystem2D>();

    ecs.RegisterSystem<PhysicsSystem2D>(&ecs);
    ecs.AddComponentToSystem<Transform2D, PhysicsSystem2D>();
    ecs.AddComponentToSystem<Physics2D, PhysicsSystem2D>();

    g_inputMap.Bind("spawn_single", GLFW_MOUSE_BUTTON_LEFT);
    g_inputMap.Bind("spawn_continuous", GLFW_MOUSE_BUTTON_RIGHT);

    int windowWidth, windowHeight;
    g_window.GetSize(&windowWidth, &windowHeight);

    Vector2 spawnPoint = { 100.0f, 100.0f };
    std::vector<EntityID> spawnedBalls;

    const double targetFrameTime = 1.0 / 60.0;
    double lastTime = glfwGetTime();

    double fpsTimer = 0.0;
    int frameCount = 0;

    while (g_running) {
        double currentTime = glfwGetTime();

        double frameStartTime = currentTime;
        g_deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        fpsTimer += g_deltaTime;
        frameCount++;

        if (fpsTimer >= 1.0) {
            double actualFPS = (double)frameCount / fpsTimer;
            printf("[Engine Status] Active Balls: %zu | Performance Render Rate: %.2f FPS\n", spawnedBalls.size(), actualFPS);
            fpsTimer = 0.0;
            frameCount = 0;
        }

        if (g_deltaTime > 0.1) g_deltaTime = targetFrameTime;

        g_window.PollEvents();
        if (g_window.ShouldClose()) g_running = false;

        g_window.GetSize(&windowWidth, &windowHeight);
        g_inputMap.Update(g_window.GetNativeWindow());

        float substepDelta = (float)g_deltaTime / 6.0f;

        // 360Hz substep loop
        for (int substep = 0; substep < 6; substep++)
        {
            for (EntityID ballId : spawnedBalls) {
                auto* ballPhysics = ecs.GetComponent<Physics2D>(ballId);
                if (ballPhysics) {
                    ballPhysics->ApplyForce(Vector2(0.0f, GLOBAL_GRAVITY * ballPhysics->GetMass()));
                }
            }

            ecs.GetSystem<PhysicsSystem2D>()->Update(substepDelta);
            ecs.GetSystem<CollisionSystem2D>()->Update(substepDelta);

            // Window boundary constraints
            for (EntityID ballId : spawnedBalls) {
                auto* trans = ecs.GetComponent<Transform2D>(ballId);
                auto* col = ecs.GetComponent<Collider2D>(ballId);
                auto* phys = ecs.GetComponent<Physics2D>(ballId);

                if (trans && col && phys) {
                    float radius = col->GetRadius(trans->scale.x, trans->scale.y);

                    // Left Wall
                    if (trans->position.x - radius < 0.0f) {
                        trans->position.x = radius;
                        if (phys->velocity.x < 0.0f) phys->velocity.x = -phys->velocity.x * phys->restitution;
                    }
                    // Right Wall
                    else if (trans->position.x + radius > (float)windowWidth) {
                        trans->position.x = (float)windowWidth - radius;
                        if (phys->velocity.x > 0.0f) phys->velocity.x = -phys->velocity.x * phys->restitution;
                    }

                    // Bottom Wall
                    if (trans->position.y + radius > (float)windowHeight) {
                        trans->position.y = (float)windowHeight - radius;
                        if (phys->velocity.y > 0.0f) phys->velocity.y = -phys->velocity.y * phys->restitution;
                    }
                    // Top Wall
                    else if (trans->position.y - radius < 0.0f) {
                        trans->position.y = radius;
                        if (phys->velocity.y < 0.0f) phys->velocity.y = -phys->velocity.y * phys->restitution;
                    }
                }
            }
        }

        auto spawnBallWithImpulse = [&]() {
            int mouseX, mouseY;
            g_inputMap.GetLocalCursorPosition(&mouseX, &mouseY);

            Vector2 cursorCoords = { (float)mouseX, (float)mouseY };
            Vector2 launchDirection = cursorCoords - spawnPoint;
            float distance = Vector2::Magnitude(launchDirection);

            if (distance > 0.0f) {
                Vector2 normalizedDir = Vector2::Normalize(launchDirection);
                Vector2 initialVelocity = normalizedDir * distance;

                EntityID ball = ecs.CreateEntity();
                ecs.AddComponent<Transform2D>(ball, Transform2D(spawnPoint, 0.0f, { 1.0f, 1.0f }));
                ecs.AddComponent<Collider2D>(ball, Collider2D({ BALL_RADIUS, BALL_RADIUS }, BALL_RADIUS, true));
                ecs.AddComponent<Physics2D>(ball, Physics2D(1.2f, false, 0.2f, 0.7f));

                ecs.GetComponent<Physics2D>(ball)->velocity = initialVelocity;

                spawnedBalls.push_back(ball);
            }
            };

        if (g_inputMap.IsButtonPressed("spawn_single"))   spawnBallWithImpulse();
        if (g_inputMap.IsButtonDown("spawn_continuous")) spawnBallWithImpulse();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
            0, (float)windowWidth, (float)windowHeight, 0, -100.0f, 100.0f
        );

        g_renderer.Begin(projection);

        // Rendering pass
        for (EntityID ballId : spawnedBalls) {
            auto* trans = ecs.GetComponent<Transform2D>(ballId);
            auto* col = ecs.GetComponent<Collider2D>(ballId);
            if (trans && col) {
                Vector2 center = col->GetGlobalCenter(trans);
                float radius = col->GetRadius(trans->scale.x, trans->scale.y);
                g_renderer.DrawCircle(center, radius, Color::CYAN, true);
            }
        }

        g_renderer.Flush();
        g_window.SwapBuffers();

        while (glfwGetTime() < frameStartTime + targetFrameTime)
            std::this_thread::yield();
    }

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    JPH::UnregisterTypes();

    g_textureLoader.Cleanup();
    g_window.Exit();
    return 0;
}