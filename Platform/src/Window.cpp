#include <Platform/Window.h>
#include <cstdio>

bool Window::Init(int w, int h, const char* title) {
    if (!glfwInit()) {
        fprintf(stderr, "[ERROR]: Failed to initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    m_window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!m_window) {
        fprintf(stderr, "[ERROR]: Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    printf("[INFO]: GLFW window created\n");

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "[ERROR]: Failed to initialize GLAD\n");
        return false;
    }

    printf("[INFO]: GLAD initialized\n");

    return true;
}

void Window::Exit() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
    printf("[INFO]: Window destroyed\n");
}

void Window::PollEvents() {
    glfwPollEvents();
}

bool Window::ShouldClose() {
    return glfwWindowShouldClose(m_window);
}

void Window::SwapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::GetSize(int* w, int* h) {
    glfwGetWindowSize(m_window, w, h);
}