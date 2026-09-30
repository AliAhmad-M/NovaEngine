#pragma once
#include <Core/Utilities/Vector2.h>
#include <Core/Behavior/Inspectables.h>
#include <array>
#include <algorithm>

namespace Nova
{
    /**
     * @brief A 2D camera component for viewport management
     */
    struct Camera2D
    {
        bool isMain;

        /**
         * @brief Construct a new Camera 2D object
         * @param isMainCamera Whether this is the primary camera
         * @param m_zoom Initial zoom level
         * @param m_boundaries Camera movement limits {left, right, top, bottom}
         */
        Camera2D(bool isMain = false,
            float m_zoom = 1.0f,
            std::array<float, 4> m_boundaries = { -INFINITY, INFINITY, -INFINITY, INFINITY })
        {
            SetZoom(m_zoom);
            SetBoundaries(m_boundaries);
        }

        void ReflectFields(Inspectables& fields)
        {
            fields.Bool("Main Camera", &isMain);
            fields.Vec2("Offset", [this] { return GetOffset(); }, [this](Vector2 v) { SetOffset(v); });

            fields.Header("View");
            fields.Float("Zoom", [this] { return GetZoom(); }, [this](float v) { SetZoom(v); }, 0.01f);
            fields.Int("Viewport Width", [this] { int w, h; GetViewPortSize(w, h); return w; },
                [this](int v) { int w, h; GetViewPortSize(w, h); SetViewportSize(v, h); });
            fields.Int("Viewport Height", [this] { int w, h; GetViewPortSize(w, h); return h; },
                [this](int v) { int w, h; GetViewPortSize(w, h); SetViewportSize(w, v); });

            fields.Header("Boundaries");
            fields.Float("Left Bound", [this] { return GetBoundaries()[0]; },
                [this](float v) { auto b = GetBoundaries(); b[0] = v; SetBoundaries(b); }, 1.0f, 0.0f, 0.0f, true, -1.0f);
            fields.Float("Right Bound", [this] { return GetBoundaries()[1]; },
                [this](float v) { auto b = GetBoundaries(); b[1] = v; SetBoundaries(b); }, 1.0f, 0.0f, 0.0f, true, 1.0f);
            fields.Float("Top Bound", [this] { return GetBoundaries()[2]; },
                [this](float v) { auto b = GetBoundaries(); b[2] = v; SetBoundaries(b); }, 1.0f, 0.0f, 0.0f, true, -1.0f);
            fields.Float("Bottom Bound", [this] { return GetBoundaries()[3]; },
                [this](float v) { auto b = GetBoundaries(); b[3] = v; SetBoundaries(b); }, 1.0f, 0.0f, 0.0f, true, 1.0f);
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
            m_width = std::max(1, width);
            m_height = std::max(1, height);
        }

    private:
        int m_width = 1280;
        int m_height = 720;

        float m_zoom;
        Vector2 m_offset = Vector2::ZERO;
        std::array<float, 4> m_boundaries;
    };
}