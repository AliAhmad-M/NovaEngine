#include <thread>
#include <cstdio>
#include <string>

#include <Core/ECS/ECS.h>
#include <Core/Components.h>
#include <Platform/Window.h>
#include <Platform/InputMap.h>
#include <Graphics/Renderer.h>
#include <Graphics/Texture/TextureLoader.h>
#include <Graphics/Font.h>
#include <Graphics/Camera/Camera2D.h>
#include <Graphics/Components.h>
#include <Graphics/Systems.h>
#include <Physics/Components.h>
#include <Physics/Systems.h>
#include <Runtime/Scene/SceneManager.h>

#include <Test/Scenes/DynamicsTest.h>
#include <Test/Scenes/GraphicsTest.h>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

// Global engine context
Window        g_window;
Renderer      g_renderer;
InputMap      g_inputMap;
TextureLoader g_textureLoader;
Font          g_engineFont;
ECS           g_ecs;
Camera2D      g_camera;
bool          g_running = true;
double        g_deltaTime = 0;

int main()
{
    std::string defaultScene = "Graphics";

    if (!g_window.Init(1280, 720, "Star Platinum Engine")) return -1;
    g_renderer.Init();

    if (!g_engineFont.LoadFromFile("project/assets/boldpixels.ttf", 24.0f)) {
        fprintf(stderr, "[ERROR]: Failed to load boldpixels.ttf\n");
    }

    // Initialize ECS and components
    g_ecs.Init();
    g_ecs.RegisterComponent<Transform2D>();
    g_ecs.RegisterComponent<Sprite2D>();
    g_ecs.RegisterComponent<Collider2D>();
    g_ecs.RegisterComponent<Physics2D>();

    // Initialize systems
    g_ecs.RegisterSystem<SpriteSystem>(&g_ecs, &g_camera, &g_window, &g_renderer);
    g_ecs.AddComponentToSystem<Transform2D, SpriteSystem>();
    g_ecs.AddComponentToSystem<Sprite2D, SpriteSystem>();

    g_ecs.RegisterSystem<CollisionSystem2D>(&g_ecs);
    g_ecs.AddComponentToSystem<Transform2D, CollisionSystem2D>();
    g_ecs.AddComponentToSystem<Collider2D, CollisionSystem2D>();

    g_ecs.RegisterSystem<PhysicsSystem2D>(&g_ecs);
    g_ecs.AddComponentToSystem<Transform2D, PhysicsSystem2D>();
    g_ecs.AddComponentToSystem<Physics2D, PhysicsSystem2D>();

    // Initialize Jolt physics
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    SceneManager sceneManager;
    sceneManager.CreateScene<DynamicsTest>("Physics");
    sceneManager.CreateScene<GraphicsTest>("Graphics");
    sceneManager.ChangeScene(defaultScene);

    const double targetFrameTime = 1.0 / 60.0;
    double lastTime = glfwGetTime();

    while (g_running) {
        double currentTime = glfwGetTime();
        double frameStartTime = currentTime;
        g_deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (g_deltaTime > 0.1) g_deltaTime = targetFrameTime;

        g_window.PollEvents();
        if (g_window.ShouldClose()) g_running = false;

        g_inputMap.Update(g_window.GetNativeWindow());
        
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update the active scene
        sceneManager.UpdateScene((float)g_deltaTime);

        g_window.SwapBuffers();

        while (glfwGetTime() < frameStartTime + targetFrameTime)
            std::this_thread::yield();
    }

    // Clear Jolt instance
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    JPH::UnregisterTypes();

    // Clear the engine context
    g_ecs.DeleteAllEntities();
    g_textureLoader.Cleanup();
    g_window.Exit();
    return 0;
}