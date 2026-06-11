#pragma once
#include <array>
#include <Core/Utilities/Vector2.h>
#include <Graphics/Camera/ICamera.h>
#include <Platform/Window.h>

/**
 * @brief Manages a 2D viewport with positional clamping.
 */
class Camera2D : public ICamera 
{
public:
    /**
     * @brief Initializes camera with window, optional bounds and offset.
     * @param window     Pointer to the active Window.
     * @param boundaries Array of { left, right, up, down }.
     * @param offset     Initial camera position.
     */
    Camera2D( 
        const std::array<float, 4>& boundaries = { -INFINITY, INFINITY, -INFINITY, INFINITY },
        const Vector2& offset = { 0.0f, 0.0f }
    ) : m_boundaries(boundaries), m_offset(offset) {}

    /**
     * @brief Sets the camera offset.
     * @param offset New position vector.
     */
    void SetOffset(Window* window, const Vector2& offset);

    /**
     * @brief Gets the camera offset.
     * @return Reference to the current offset vector.
     */
    Vector2& GetOffset();

    /**
     * @brief Sets world boundaries for camera movement.
     * @param left, right, up, down Boundary coordinates.
     */
    void SetBoundaries(Window* window, float left, float right, float up, float down);

private:
    std::array<float, 4> m_boundaries;
    Vector2 m_offset;
};