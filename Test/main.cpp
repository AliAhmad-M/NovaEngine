#include <thread>
#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Core/ECS/ECS.h>
#include <Core/Components.h>
#include <Graphics/Components.h>
#include <Graphics/Systems/SpriteSystem.h>
#include <Graphics/Camera/Camera2D.h>
#include <Platform/Window.h>
#include <Graphics/Renderer.h>
#include <Graphics/Texture/TextureLoader.h>

Window        g_window;
Renderer      g_renderer;
TextureLoader g_textureLoader;
bool          g_running = true;
double        g_deltaTime = 0;

const float SLIME_SPEED = 100.0f;

int main() {
    if (!g_window.Init(1280, 720, "Slime Test"))
        return -1;

    g_renderer.Init();

    ECS ecs;
    ecs.Init();

    ecs.RegisterComponent<Transform2D>();
    ecs.RegisterComponent<Sprite2D>();

    ecs.RegisterSystem<SpriteSystem>();
    ecs.AddComponentToSystem<Transform2D, SpriteSystem>();
    ecs.AddComponentToSystem<Sprite2D, SpriteSystem>();

    Camera2D camera;

    Texture* slimeTex = g_textureLoader.LoadTexture("assets/slime.png");

    EntityID slime = ecs.CreateEntity();

    ecs.AddComponent<Transform2D>(slime, Transform2D(
        { 600.0f, 320.0f },
        0.0f,
        { 4.0f, 4.0f }
    ));

    Sprite2D sprite(slimeTex, 4, 4, 0);

    sprite.AddAnimation("idle_down",  { 0 }, 1);
    sprite.AddAnimation("idle_up",    { 1 }, 1);
    sprite.AddAnimation("idle_left",  { 2 }, 1);
    sprite.AddAnimation("idle_right", { 3 }, 1);
    sprite.AddAnimation("walk_down",  { 0, 4, 8,  12 }, 8);
    sprite.AddAnimation("walk_up",    { 1, 5, 9,  13 }, 8);
    sprite.AddAnimation("walk_left",  { 2, 6, 10, 14 }, 8);
    sprite.AddAnimation("walk_right", { 3, 7, 11, 15 }, 8);

    sprite.SetAnimation("idle_down");

    ecs.AddComponent<Sprite2D>(slime, sprite);

    const char* lastIdleAnim = "idle_down";
    const double targetFrameTime = 1.0 / 60.0;
    double lastTime = glfwGetTime();

    while (g_running) {
        double currentTime = glfwGetTime();
        g_deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        g_window.PollEvents();
        if (g_window.ShouldClose()) g_running = false;

        // Input
        GLFWwindow* nativeWindow = g_window.GetNativeWindow();
        Transform2D* transform = ecs.GetComponent<Transform2D>(slime);
        Sprite2D* sp = ecs.GetComponent<Sprite2D>(slime);

        float dx = 0.0f, dy = 0.0f;

        if (glfwGetKey(nativeWindow, GLFW_KEY_LEFT) == GLFW_PRESS) dx -= 1.0f;
        if (glfwGetKey(nativeWindow, GLFW_KEY_RIGHT) == GLFW_PRESS) dx += 1.0f;
        if (glfwGetKey(nativeWindow, GLFW_KEY_UP) == GLFW_PRESS) dy -= 1.0f;
        if (glfwGetKey(nativeWindow, GLFW_KEY_DOWN) == GLFW_PRESS) dy += 1.0f;

        // Normalise diagonal
        if (dx != 0.0f && dy != 0.0f) {
            dx *= 0.7071f;
            dy *= 0.7071f;
        }

        transform->position.x += dx * SLIME_SPEED * (float)g_deltaTime;
        transform->position.y += dy * SLIME_SPEED * (float)g_deltaTime;

        // Animation — horizontal takes priority
        if (dx < 0.0f) { sp->SetAnimation("walk_left");  lastIdleAnim = "idle_left"; }
        else if (dx > 0.0f) { sp->SetAnimation("walk_right"); lastIdleAnim = "idle_right"; }
        else if (dy < 0.0f) { sp->SetAnimation("walk_up");    lastIdleAnim = "idle_up"; }
        else if (dy > 0.0f) { sp->SetAnimation("walk_down");  lastIdleAnim = "idle_down"; }
        else { sp->SetAnimation(lastIdleAnim); }

        // Render
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int windowWidth, windowHeight;
        g_window.GetSize(&windowWidth, &windowHeight);

        Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
            0, (float)windowWidth, (float)windowHeight, 0, -1.0f, 1.0f
        );

        g_renderer.Begin(projection);
        ecs.GetSystem<SpriteSystem>()->Draw(&camera, &g_window, &g_renderer, &ecs);
        g_renderer.Flush();

        g_window.SwapBuffers();

        while (glfwGetTime() < lastTime + targetFrameTime)
            std::this_thread::yield();
    }

    g_textureLoader.Cleanup();
    g_window.Exit();
    return 0;
}