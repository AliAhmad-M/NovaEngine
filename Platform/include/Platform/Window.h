#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Nova 
{
    class Window
    {
    public:
        /**
         * @brief Initializes GLFW and creates a window
         *
         * @param w Window width in pixels
         * @param h Window height in pixels
         * @param title Window title text
         * @return true if successful, false if failed
         */
        bool Init(int w, int h, const char* title);

        /**
         * @brief Destroys window and shuts down GLFW
         */
        void Exit();

        /**
         * @brief Processes pending window events
         */
        void PollEvents();

        /**
         * @brief Checks if window close was requested
         *
         * @return true if window should close
         */
        bool ShouldClose();

        /**
         * @brief Swaps the front and back buffers
         */
        void SwapBuffers();

        /**
         * @brief Returns the native GLFW window handle
         *
         * @return Pointer to GLFWwindow
         */
        GLFWwindow* GetNativeWindow() { return m_window; }

        /**
         * @brief Gets the current window width
         */
        int GetWidth();

        /**
         * @brief Gets the current window height
         */
        int GetHeight();

        /**
         * @brief Gets the current window dimensions
         *
         * @param w Output pointer for width
         * @param h Output pointer for height
         */
        void GetSize(int* w, int* h);

    private:
        GLFWwindow* m_window = nullptr;
    };
}