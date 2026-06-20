#pragma once
#include <Core/Utilities/Vector2.h>
#include <array>
#include <algorithm>

namespace Nova
{
    /**
     * @brief A 2D camera component for viewport management
     */
    struct Camera2D
    {
        bool isMainCamera;

        /**
         * @brief Construct a new Camera 2D object
         * @param isMainCamera Whether this is the primary camera
         * @param m_zoom Initial zoom level
         * @param m_boundaries Camera movement limits {left, right, top, bottom}
         */
        Camera2D(bool isMainCamera = false,
            float m_zoom = 1.0f,
            std::array<float, 4> m_boundaries = { -INFINITY, INFINITY, -INFINITY, INFINITY })
        {
            SetZoom(m_zoom);
            SetBoundaries(m_boundaries);
        }

        /**
         * @brief Get the current zoom level
         * @return Current zoom float
         */
        float GetZoom() const
        {
            return m_zoom;
        }

        /**
         * @brief Set the zoom level (clamped to a minimum of 0.01)
         * @param newZoom The desired zoom level
         */
        void SetZoom(float newZoom)
        {
            m_zoom = std::max(0.01f, newZoom);
        }

        /**
         * @brief Get the current camera movement boundaries
         * @return Array of {left, right, top, bottom} boundaries
         */
        std::array<float, 4> GetBoundaries() const
        {
            return m_boundaries;
        }

        /**
         * @brief Set camera boundaries. Adjusts if boundaries are smaller than viewport
         * @param newBoundaries Array of {left, right, top, bottom} constraints
         */
        void SetBoundaries(std::array<float, 4> newBoundaries)
        {
            float left = newBoundaries[0];
            float right = newBoundaries[1];
            float up = newBoundaries[2];
            float down = newBoundaries[3];

            float boundaryWidth = right - left;
            float boundaryHeight = down - up;

            // Make sure boundaries are not less than viewport size
            if (boundaryWidth < (float)m_width)
            {
                float padding = ((float)m_width - boundaryWidth) * 0.5f;
                left -= padding;
                right += padding;
            }
            if (boundaryHeight < (float)m_height)
            {
                float padding = ((float)m_height - boundaryHeight) * 0.5f;
                up -= padding;
                down += padding;
            }

            m_boundaries = { left, right, up, down };
        }

        /**
         * @brief Get the current camera offset (position)
         * @return The camera's current coordinate offset
         */
        const Vector2& GetOffset() const
        {
            return m_offset;
        }

        /**
         * @brief Set the camera offset, clamped within defined boundaries
         * @param newOffset The desired position
         */
        void SetOffset(const Vector2& newOffset)
        {
            float cameraLeft = newOffset.x;
            float cameraRight = newOffset.x + (float)m_width;
            float cameraTop = newOffset.y;
            float cameraBottom = newOffset.y + (float)m_height;

            Vector2 clampedOffset = newOffset;

            // Check left boundary
            if (cameraLeft < m_boundaries[0])
            {
                clampedOffset.x = m_boundaries[0];
            }

            // Check right boundary  
            if (cameraRight > m_boundaries[1])
            {
                clampedOffset.x = m_boundaries[1] - (float)m_width;
            }

            // Check top boundary
            if (cameraTop < m_boundaries[2])
            {
                clampedOffset.y = m_boundaries[2];
            }

            // Check bottom boundary
            if (cameraBottom > m_boundaries[3])
            {
                clampedOffset.y = m_boundaries[3] - (float)m_height;
            }

            // Set the clamped offset
            m_offset = clampedOffset;
        }

        /**
         * @brief Get the current viewport dimensions
         * @param width Output parameter for width
         * @param height Output parameter for height
         */
        void GetViewPortSize(int& outWidth, int& outHeight) const
        {
            outWidth = m_width;
            outHeight = m_height;
        }

        /**
         * @brief Set the viewport dimensions
         * @param width New width in pixels
         * @param height New height in pixels
         */
        void SetViewportSize(int width, int height)
        {
            m_width = width;
            m_height = height;
        }

    private:
        int m_width = 1280;
        int m_height = 720;

        float m_zoom;
        Vector2 m_offset = Vector2::ZERO;
        std::array<float, 4> m_boundaries;
    };
}