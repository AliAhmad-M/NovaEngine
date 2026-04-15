#include <thread>
#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Core/ECS/ECS.h>
#include <Platform/Window.h>
#include <Graphics/Renderer.h>

// Global engine state

Window        g_window;
Renderer      g_renderer;
bool   g_running = true;
double g_deltaTime = 0;

int main() {
    // Init window & GLAD
    if (!g_window.Init(1280, 720, "Window Test")) {
        return -1;
    }

    // Init renderer
    g_renderer.Init();

    // Main Loop
    double lastTime = glfwGetTime();
    const double targetFrameTime = 1.0 / 60.0;

    while (g_running) {
        double currentTime = glfwGetTime();
        g_deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Standard frame logic
        g_window.PollEvents();
        if (g_window.ShouldClose()) g_running = false;

        // Clear Screen
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int windowWidth, windowHeight;
        g_window.GetSize(&windowWidth, &windowHeight);

        // Projection
        Matrix4x4 projection = Matrix4x4::CreateOrthographicOffCenter(
            0, (float)windowWidth, (float)windowHeight, 0, -1.0f, 1.0f
        );
        
        // Rendering
        g_renderer.Begin(projection);

        g_renderer.Flush();
        g_window.SwapBuffers();

        // 60 FPS
        while (glfwGetTime() < lastTime + targetFrameTime) {
            std::this_thread::yield();
        }
    }

    // Cleanup
    g_window.Exit();

    return 0;
}