#pragma once
#include <unordered_map>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Nova 
{
    class InputMap
    {
    public:
        /**
         * @brief Binds a string action name to a physical GLFW key
         *
         * @param action The name of the action
         * @param glfwKey The GLFW key code (e.g., GLFW_KEY_SPACE)
         */
        void Bind(const std::string& action, int glfwKey);

        /**
         * @brief Polls and updates key and mouse states for the frame
         *
         * @param window Pointer to the active GLFW window
         */
        void Update(GLFWwindow* window);

        /**
         * @brief Checks if the action's key is currently held down
         *
         * @return true if the key is down
         */
        bool IsButtonDown(const std::string& action)     const;

        /**
         * @brief Checks if the action's key is currently up
         *
         * @return true if the key is up
         */
        bool IsButtonUp(const std::string& action)       const;

        /**
         * @brief Checks if the action's key was pressed exactly this frame
         *
         * @return true if transitioned from up to down
         */
        bool IsButtonPressed(const std::string& action)  const;

        /**
         * @brief Checks if the action's key was released exactly this frame
         *
         * @return true if transitioned from down to up
         */
        bool IsButtonReleased(const std::string& action) const;

        /**
         * @brief Gets mouse coordinates relative to the screen
         *
         * @param x Output pointer for screen X coordinate
         * @param y Output pointer for screen Y coordinate
         */
        void GetGlobalMousePosition(int* x, int* y) const;

        /**
         * @brief Gets mouse coordinates relative to the window client area
         *
         * @param x Output pointer for window X coordinate
         * @param y Output pointer for window Y coordinate
         */
        void GetLocalCursorPosition(int* x, int* y) const;

    private:
        struct KeyState {
            bool current = false;
            bool previous = false;
        };

        GLFWwindow* m_window = nullptr;
        std::unordered_map<std::string, int>      m_bindings;
        std::unordered_map<std::string, KeyState> m_states;

        int m_globalMouseX = 0, m_globalMouseY = 0;
        int m_localMouseX = 0, m_localMouseY = 0;
    };
}