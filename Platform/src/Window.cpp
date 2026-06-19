#include <Platform/Window.h>
#include <Core/EngineDebug.h>

bool Window::Init(int w, int h, const char* title) 
{
    if (!glfwInit()) 
    {
        EngineDebug::LogError("Failed to initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    m_window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!m_window) 
    {
        EngineDebug::LogError("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    EngineDebug::LogInfo("GLFW window created");

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    {
        EngineDebug::LogError("Failed to initialize GLAD");
        return false;
    }

    EngineDebug::LogInfo("GLAD initialized");
    return true;
}

void Window::Exit() 
{
    if (m_window) 
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
    EngineDebug::LogInfo("Window destroyed");
}

void Window::PollEvents() 
{
    glfwPollEvents();
}

bool Window::ShouldClose() 
{
    return glfwWindowShouldClose(m_window);
}

void Window::SwapBuffers() 
{
    glfwSwapBuffers(m_window);
}

void Window::GetSize(int* w, int* h) 
{
    glfwGetWindowSize(m_window, w, h);
}