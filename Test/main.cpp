#include <thread>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <memory>
#include <string>

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
#include <Runtime/Scene/IScene.h>
#include <Runtime/Scene/SceneManager.h>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

// Universal Engine Engine Contexts/Globals
Window        g_window;
Renderer      g_renderer;
InputMap      g_inputMap;
TextureLoader g_textureLoader;
bool          g_running = true;
double        g_deltaTime = 0;

#define METER 32

// Sandbox scene implementation
class SandboxScene : public IScene
{
public:
    // Called when the scene is loaded in
    void Ready() override
    {
        const float GLOBAL_GRAVITY = 981.0f / METER;

        m_ecs.Init();

        m_ecs.RegisterComponent<Transform2D>();
        m_ecs.RegisterComponent<Sprite2D>();
        m_ecs.RegisterComponent<Collider2D>();
        m_ecs.RegisterComponent<Physics2D>();

        m_ecs.RegisterSystem<SpriteSystem>(&m_ecs, &m_camera, &g_window, &g_renderer);
        m_ecs.AddComponentToSystem<Transform2D, SpriteSystem>();
        m_ecs.AddComponentToSystem<Sprite2D, SpriteSystem>();

        m_ecs.RegisterSystem<CollisionSystem2D>(&m_ecs);
        m_ecs.AddComponentToSystem<Transform2D, CollisionSystem2D>();
        m_ecs.AddComponentToSystem<Collider2D, CollisionSystem2D>();

        m_ecs.RegisterSystem<PhysicsSystem2D>(&m_ecs);
        m_ecs.AddComponentToSystem<Transform2D, PhysicsSystem2D>();
        m_ecs.AddComponentToSystem<Physics2D, PhysicsSystem2D>();

        g_inputMap.Bind("spawn_single", GLFW_MOUSE_BUTTON_LEFT);
        g_inputMap.Bind("spawn_continuous", GLFW_MOUSE_BUTTON_RIGHT);

        printf("[INFO]: SandboxScene initialized and components registered.\n");
    }

    // Called every single frame step
    void Update(float dt) override
    {
        // Metrics for debugging performance
        static double fpsTimer = 0.0;
        static int frameCount = 0;

        fpsTimer += dt;
        frameCount++;

        if (fpsTimer >= 1.0) {
            double actualFPS = (double)frameCount / fpsTimer;
            printf("[Engine Status] Active Balls: %zu | Performance Render Rate: %.2f FPS\n", m_spawnedBalls.size(), actualFPS);
            fpsTimer = 0.0;
            frameCount = 0;
        }

        const float GLOBAL_GRAVITY = 981.0f / METER;
        int windowWidth, windowHeight;
        g_window.GetSize(&windowWidth, &windowHeight);

        // Core input handling
        if (g_inputMap.IsButtonPressed("spawn_single"))   SpawnBallWithImpulse();
        if (g_inputMap.IsButtonDown("spawn_continuous")) SpawnBallWithImpulse();

        // 360Hz physics substep
        float substepDelta = dt / 6.0f;
        for (int substep = 0; substep < 6; substep++)
        {
            for (EntityID ballId : m_spawnedBalls) {
                auto* ballPhysics = m_ecs.GetComponent<Physics2D>(ballId);
                if (ballPhysics) {
                    ballPhysics->ApplyForce(Vector2(0.0f, GLOBAL_GRAVITY * ballPhysics->GetMass()));
                }
            }

            m_ecs.GetSystem<PhysicsSystem2D>()->Update(substepDelta);
            m_ecs.GetSystem<CollisionSystem2D>()->Update(substepDelta);

            // Handle hard boundary constraints
            ApplyWindowConstraints(windowWidth, windowHeight);
        }

        // Render setup
        Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
            0, (float)windowWidth, (float)windowHeight, 0, -100.0f, 100.0f
        );

        g_renderer.Begin(projection);
        for (EntityID ballId : m_spawnedBalls) {
            auto* trans = m_ecs.GetComponent<Transform2D>(ballId);
            auto* col = m_ecs.GetComponent<Collider2D>(ballId);
            if (trans && col) {
                Vector2 center = col->GetGlobalCenter(trans);
                float radius = col->GetRadius(trans->scale.x, trans->scale.y);
                g_renderer.DrawCircle(center, radius, Color::CYAN, true);
            }
        }
        g_renderer.Flush();
    }

    // Called when changing scenes or deleting this scene
    void Leave() override
    {
        // Drop all entity records instantly
        m_spawnedBalls.clear();

        // Clear scene entities
        m_ecs.DeleteAllEntities();

        printf("[INFO]: SandboxScene successfully completely cleaned and unloaded.\n");
    }

    size_t GetBallCount() const { return m_spawnedBalls.size(); }

private:
    ECS m_ecs;
    Camera2D m_camera;
    std::vector<EntityID> m_spawnedBalls;

    Vector2 spawnPoint = { 100.0f, 100.0f };
    const float BALL_RADIUS = 8.0f;

    void SpawnBallWithImpulse()
    {
        int mouseX, mouseY;
        g_inputMap.GetLocalCursorPosition(&mouseX, &mouseY);

        Vector2 cursorCoords = { (float)mouseX, (float)mouseY };
        Vector2 launchDirection = cursorCoords - spawnPoint;
        float distance = Vector2::Magnitude(launchDirection);

        if (distance > 0.0f) {
            Vector2 normalizedDir = Vector2::Normalize(launchDirection);
            Vector2 initialVelocity = normalizedDir * distance;

            EntityID ball = m_ecs.CreateEntity();
            m_ecs.AddComponent<Transform2D>(ball, Transform2D(spawnPoint, 0.0f, { 1.0f, 1.0f }));
            m_ecs.AddComponent<Collider2D>(ball, Collider2D({ BALL_RADIUS, BALL_RADIUS }, BALL_RADIUS, true));
            m_ecs.AddComponent<Physics2D>(ball, Physics2D(1.2f, false, 0.2f, 0.7f));

            m_ecs.GetComponent<Physics2D>(ball)->velocity = initialVelocity;

            m_spawnedBalls.push_back(ball);
        }
    }

    void ApplyWindowConstraints(int windowWidth, int windowHeight)
    {
        for (EntityID ballId : m_spawnedBalls) {
            auto* trans = m_ecs.GetComponent<Transform2D>(ballId);
            auto* col = m_ecs.GetComponent<Collider2D>(ballId);
            auto* phys = m_ecs.GetComponent<Physics2D>(ballId);

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
};

// Main app entry point
int main()
{
    // Initialze window
    if (!g_window.Init(1280, 720, "Engine Test")) 
    {
        return -1;
    }

    // Initialize renderer
    g_renderer.Init();

    // Global physics layout allocators
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    // Setup scene manager
    SceneManager sceneManager;
    sceneManager.CreateScene<SandboxScene>("Sandbox");
    sceneManager.ChangeScene("Sandbox");

    const double targetFrameTime = 1.0 / 60.0;
    double lastTime = glfwGetTime();

    double fpsTimer = 0.0;
    int frameCount = 0;

    // Engine loop
    while (g_running) {
        double currentTime = glfwGetTime();
        double frameStartTime = currentTime;
        g_deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        fpsTimer += g_deltaTime;
        frameCount++;

        if (g_deltaTime > 0.1) g_deltaTime = targetFrameTime;

        g_window.PollEvents();
        if (g_window.ShouldClose()) g_running = false;

        g_inputMap.Update(g_window.GetNativeWindow());

        // Prepare context rendering buffers
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update active scene
        sceneManager.UpdateScene((float)g_deltaTime);

        g_window.SwapBuffers();

        // Target pacing locks
        while (glfwGetTime() < frameStartTime + targetFrameTime)
            std::this_thread::yield();
    }

    // Cleanup resources
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    JPH::UnregisterTypes();

    g_textureLoader.Cleanup();
    g_window.Exit();
    return 0;
}