#include <Graphics/Camera/Camera2D.h>
#include <cstdio>

void Camera2D::SetOffset(Window* window, const Vector2& newOffset)
{
    if (!window)
    {
        fprintf(stderr, "[WARN]: Cannot update camera2D offset, window is null\n");
        return;
    }

    // Get viewport size

    int windowWidth;
    int windowHeight;

    window->GetSize(&windowWidth, &windowHeight);

    // Calculate the camera bounds

    float cameraLeft = newOffset.x;
    float cameraRight = newOffset.x + (float)windowWidth;
    float cameraTop = newOffset.y;
    float cameraBottom = newOffset.y + (float)windowHeight;

    Vector2 clampedOffset = newOffset;

    // Check left boundary
    if (cameraLeft < m_boundaries[0])
    {
        clampedOffset.x = m_boundaries[0];
    }

    // Check right boundary  
    if (cameraRight > m_boundaries[1])
    {
        clampedOffset.x = m_boundaries[1] - (float)windowWidth;
    }

    // Check top boundary
    if (cameraTop < m_boundaries[2])
    {
        clampedOffset.y = m_boundaries[2];
    }

    // Check bottom boundary
    if (cameraBottom > m_boundaries[3])
    {
        clampedOffset.y = m_boundaries[3] - (float)windowHeight;
    }

    // Set the clamped offset
    m_camOffset = clampedOffset;
}

Vector2& Camera2D::GetOffset()
{
    return m_camOffset;
}

float Camera2D::GetZoom()
{
	return m_zoom;
}

void Camera2D::SetZoom(float zoom)
{
	m_zoom = std::max(0.01f, zoom);
}

void Camera2D::SetBoundaries(Window* window, float left, float right, float up, float down)
{
    if (left > right)
        std::swap(left, right);

    if (up > down)
        std::swap(up, down);

    // Get viewport size

    int windowWidth;
    int windowHeight;
    window->GetSize(&windowWidth, &windowHeight);

    // Make sure boundaries are not less than viewport

    float boundaryWidth = right - left;
    float boundaryHeight = down - up;

    if (boundaryWidth < (float)windowWidth)
    {
        float padding = ((float)windowWidth - boundaryWidth) * 0.5f;
        left -= padding;
        right += padding;
    }

    if (boundaryHeight < (float)windowHeight)
    {
        float padding = ((float)windowHeight - boundaryHeight) * 0.5f;
        up -= padding;
        down += padding;
    }

    m_boundaries = { left, right, up, down };
}
