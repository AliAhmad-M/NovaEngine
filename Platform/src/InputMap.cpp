#include <Platform/InputMap.h>
#include <cstdio>

void InputMap::Bind(const std::string& action, int glfwKey) {
    if (m_bindings.count(action))
        printf("[WARN]: Action '%s' already bound, overriding\n", action.c_str());

    m_bindings[action] = glfwKey;
    m_states[action] = KeyState{};
}

void InputMap::Update(GLFWwindow* window) {
    m_window = window;

    // Update key and mouse button states
    for (auto& [action, key] : m_bindings) {
        KeyState& state = m_states[action];
        state.previous = state.current;

        // Mouse buttons are 0-7, keys start at 32
        if (key <= GLFW_MOUSE_BUTTON_LAST)
            state.current = glfwGetMouseButton(window, key) == GLFW_PRESS;
        else
            state.current = glfwGetKey(window, key) == GLFW_PRESS;
    }

    // Update mouse position
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    m_localMouseX = (int)mx;
    m_localMouseY = (int)my;

    int winX, winY;
    glfwGetWindowPos(window, &winX, &winY);
    m_globalMouseX = winX + m_localMouseX;
    m_globalMouseY = winY + m_localMouseY;
}

bool InputMap::IsButtonDown(const std::string& action) const {
    if (!m_states.count(action)) { printf("[WARN]: Action '%s' not bound\n", action.c_str()); return false; }
    return m_states.at(action).current;
}

bool InputMap::IsButtonUp(const std::string& action) const {
    if (!m_states.count(action)) { printf("[WARN]: Action '%s' not bound\n", action.c_str()); return false; }
    return !m_states.at(action).current;
}

bool InputMap::IsButtonPressed(const std::string& action) const {
    if (!m_states.count(action)) { printf("[WARN]: Action '%s' not bound\n", action.c_str()); return false; }
    auto& s = m_states.at(action);
    return s.current && !s.previous;
}

bool InputMap::IsButtonReleased(const std::string& action) const {
    if (!m_states.count(action)) { printf("[WARN]: Action '%s' not bound\n", action.c_str()); return false; }
    auto& s = m_states.at(action);
    return !s.current && s.previous;
}

void InputMap::GetGlobalMousePosition(int* x, int* y) const {
    if (x) *x = m_globalMouseX;
    if (y) *y = m_globalMouseY;
}

void InputMap::GetLocalCursorPosition(int* x, int* y) const {
    if (x) *x = m_localMouseX;
    if (y) *y = m_localMouseY;
}