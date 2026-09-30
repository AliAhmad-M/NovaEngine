#include <Platform/InputMap.h>
#include <Core/Debug.h>

using namespace Nova;

void InputMap::Bind(const std::string& action, int glfwKey) 
{
    if (m_bindings.count(action))
        Debug::LogWarn("Action '%s' already bound, overriding", action.c_str());

    m_bindings[action] = glfwKey;
    m_states[action] = KeyState{};
}

void InputMap::Update(GLFWwindow* window) 
{
    m_nativeWindow = window;

    // Update key and mouse button states
    for (auto& [action, key] : m_bindings) 
    {
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

bool InputMap::IsRawInputPressed(int glfwKey) const
{
	if (glfwKey <= GLFW_MOUSE_BUTTON_LAST)
		return glfwGetMouseButton(m_nativeWindow, glfwKey) == GLFW_PRESS;
	else
		return glfwGetKey(m_nativeWindow, glfwKey) == GLFW_PRESS;
}

bool InputMap::IsRawInputReleased(int glfwKey) const
{
	if (glfwKey <= GLFW_MOUSE_BUTTON_LAST)
		return glfwGetMouseButton(m_nativeWindow, glfwKey) == GLFW_RELEASE;
	else
		return glfwGetKey(m_nativeWindow, glfwKey) == GLFW_RELEASE;
}

bool InputMap::IsRawInputJustPressed(int glfwKey) const
{
	if (!m_nativeWindow) return false;
	bool current, previous;
	if (glfwKey <= GLFW_MOUSE_BUTTON_LAST)
	{
		current = glfwGetMouseButton(m_nativeWindow, glfwKey) == GLFW_PRESS;
		previous = m_states.count(std::to_string(glfwKey)) ? m_states.at(std::to_string(glfwKey)).previous : false;
	}
	else
	{
		current = glfwGetKey(m_nativeWindow, glfwKey) == GLFW_PRESS;
		previous = m_states.count(std::to_string(glfwKey)) ? m_states.at(std::to_string(glfwKey)).previous : false;
	}
	return current && !previous;
}

bool InputMap::IsRawInputJustReleased(int glfwKey) const
{
	if (!m_nativeWindow) return false;
	bool current, previous;
	if (glfwKey <= GLFW_MOUSE_BUTTON_LAST)
	{
		current = glfwGetMouseButton(m_nativeWindow, glfwKey) == GLFW_PRESS;
		previous = m_states.count(std::to_string(glfwKey)) ? m_states.at(std::to_string(glfwKey)).previous : false;
	}
	else
	{
		current = glfwGetKey(m_nativeWindow, glfwKey) == GLFW_PRESS;
		previous = m_states.count(std::to_string(glfwKey)) ? m_states.at(std::to_string(glfwKey)).previous : false;
	}
	return !current && previous;
}

bool InputMap::IsButtonPressed(const std::string& action) const 
{
    if (!m_states.count(action)) { Debug::LogWarn("Action '%s' not bound", action.c_str()); return false; }
    return m_states.at(action).current;
}

bool InputMap::IsButtonReleased(const std::string& action) const 
{
    if (!m_states.count(action)) { Debug::LogWarn("Action '%s' not bound", action.c_str()); return false; }
    return !m_states.at(action).current;
}

bool InputMap::IsButtonJustPressed(const std::string& action) const 
{
    if (!m_states.count(action)) { Debug::LogWarn("Action '%s' not bound", action.c_str()); return false; }
    auto& s = m_states.at(action);
    return s.current && !s.previous;
}

bool InputMap::IsButtonJustReleased(const std::string& action) const 
{
    if (!m_states.count(action)) { Debug::LogWarn("Action '%s' not bound", action.c_str()); return false; }
    auto& s = m_states.at(action);
    return !s.current && s.previous;
}

float InputMap::GetRawInputAxis(int negKey, int posKey)
{
	float value = 0.0f;
	if (IsRawInputPressed(negKey)) value -= 1.0f;
	if (IsRawInputPressed(posKey)) value += 1.0f;
	return value;
}

Vector2 InputMap::GetRawInputVector(
	int negXKey, int posXKey, 
	int negYKey, int posYKey)
{
	return Vector2(GetRawInputAxis(negXKey, posXKey), GetRawInputAxis(negYKey, posYKey));
}

float InputMap::GetAxis(const std::string& neg, const std::string& pos)
{
	if (!m_states.count(neg)) { Debug::LogWarn("Action '%s' not bound", neg.c_str()); return 0.0f; }
	if (!m_states.count(pos)) { Debug::LogWarn("Action '%s' not bound", pos.c_str()); return 0.0f; }
	float value = 0.0f;
	if (IsButtonPressed(neg)) value -= 1.0f;
	if (IsButtonPressed(pos)) value += 1.0f;
	return value;
}

Vector2 InputMap::GetVector(
	const std::string& negX, const std::string& posX, 
	const std::string& negY, const std::string& posY)
{
	return Vector2(GetAxis(negX, posX), GetAxis(negY, posY));
}

void InputMap::GetGlobalMousePosition(int* x, int* y) const 
{
    if (x) *x = m_globalMouseX;
    if (y) *y = m_globalMouseY;
}

void InputMap::GetLocalCursorPosition(int* x, int* y) const 
{
    if (x) *x = m_localMouseX;
    if (y) *y = m_localMouseY;
}