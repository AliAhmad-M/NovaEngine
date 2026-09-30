#pragma once
#include <Core/Utilities/Vector2.h>

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
		* @brief Checks if the raw GLFW key is currently held down
		* @param glfwKey The GLFW key code to check
		* @return true if the key is down
        */
		bool IsRawInputPressed(int glfwKey)         const;

        /**
		* @brief Checks if the raw GLFW key is currently released
		* @param glfwKey The GLFW key code to check
		* @return true if the key is up
        */
		bool IsRawInputReleased(int glfwKey)        const;

        /**
		* @brief Checks if the raw GLFW key was pressed exactly this frame
		* @param glfwKey The GLFW key code to check
		* @return true if transitioned from up to down
        */
		bool IsRawInputJustPressed(int glfwKey)     const;

        /**
		* @brief Checks if the raw GLFW key was released exactly this frame
		* @param glfwKey The GLFW key code to check
		* @return true if transitioned from down to up
        */
		bool IsRawInputJustReleased(int glfwKey)    const;

        /**
         * @brief Checks if the action's key is currently held down
		 * @param action The name of the action to check
         * @return true if the key is down
         */
        bool IsButtonPressed(const std::string& action)     const;

        /**
         * @brief Checks if the action's key is currently released
		 * @param action The name of the action to check
         * @return true if the key is up
         */
        bool IsButtonReleased(const std::string& action)    const;

        /**
         * @brief Checks if the action's key was pressed exactly this frame
		 * @param action The name of the action to check
         * @return true if transitioned from up to down
         */
        bool IsButtonJustPressed(const std::string& action)     const;

        /**
         * @brief Checks if the action's key was released exactly this frame
		 * @param action The name of the action to check
         * @return true if transitioned from down to up
         */
        bool IsButtonJustReleased(const std::string& action)    const;

        /**
		* @brief Gets the axis value for a pair of raw GLFW keys
		* @param negKey The GLFW key code for the negative direction
		* @param posKey The GLFW key code for the positive direction
		* @return A float in the range [-1.0, 1.0] representing the axis value
        */
		float GetRawInputAxis(int negKey, int posKey);

        /**
		* @brief Gets a 2D vector based on four raw GLFW keys representing the negative and positive directions of the axes
		* @param negXKey The GLFW key code for the negative X direction
		* @param posXKey The GLFW key code for the positive X direction
		* @param negYKey The GLFW key code for the negative Y direction
		* @param posYKey The GLFW key code for the positive Y direction
		* @return A Vector2 representing the combined axis values
        */
		Vector2 GetRawInputVector(
            int negXKey, int posXKey, 
            int negYKey, int posYKey);

        /**
		* @brief Gets the axis value for a pair of actions
		* @param neg The action name for the negative direction
		* @param pos The action name for the positive direction
		* @return A float in the range [-1.0, 1.0] representing the axis value
        */
        float GetAxis(const std::string& neg, const std::string& pos);

        /**
		* @brief Gets a 2D vector based on four actions representing the negative and positive directions of the axes
		* @param negX The action name for the negative X direction
		* @param posX The action name for the positive X direction
		* @param negY The action name for the negative Y direction
		* @param posY The action name for the positive Y direction
		* @return A Vector2 representing the combined axis values
        */
		Vector2 GetVector(
            const std::string& negX, const std::string& posX, 
            const std::string& negY, const std::string& posY);

        /**
         * @brief Gets mouse coordinates relative to the screen
         * @param x Output pointer for screen X coordinate
         * @param y Output pointer for screen Y coordinate
         */
        void GetGlobalMousePosition(int* x, int* y) const;

        /**
         * @brief Gets mouse coordinates relative to the window client area
         * @param x Output pointer for window X coordinate
         * @param y Output pointer for window Y coordinate
         */
        void GetLocalCursorPosition(int* x, int* y) const;

    private:
        struct KeyState {
            bool current = false;
            bool previous = false;
        };

        GLFWwindow* m_nativeWindow = nullptr;
        std::unordered_map<std::string, int>      m_bindings;
        std::unordered_map<std::string, KeyState> m_states;

        int m_globalMouseX = 0, m_globalMouseY = 0;
        int m_localMouseX = 0, m_localMouseY = 0;
    };
}