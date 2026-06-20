#pragma once

namespace Nova
{
    /**
     * @brief Represents a 2D axis-aligned rectangle.
     */
    struct Rectangle
    {
        /**
         * @brief Default constructor initializes all values to zero.
         */
        Rectangle() = default;

        /**
         * @brief Initializes rectangle with specific dimensions.
         * @param x The horizontal position.
         * @param y The vertical position.
         * @param w The width of the rectangle.
         * @param h The height of the rectangle.
         */
        Rectangle(float x, float y, float w, float h)
            : m_x(x), m_y(y), m_w(w), m_h(h)
        {
            m_minX = x;
            m_minY = y;

            m_maxX = x + w;
            m_maxY = y + h;
        }

        /**
        * @brief Get minimum x and y coordinates (top left)
        * @param outX Reference to out x coordinates
        * @param outY Reference to out y coordinates
        */
        void GetMin(float& outX, float& outY) const
        {
            outX = m_minX;
            outY = m_minY;
        }

        /**
        * @brief Get maximum x and y coordinates (bottom right)
        * @param outX Reference to out x coordinates
        * @param outY Reference to out y coordinates
        */
        void GetMax(float& outX, float& outY) const
        {
            outX = m_maxX;
            outY = m_maxY;
        }

        /**
        * @brief Get the current rectangle coordinates
        * @param outX Reference to out x coordinates
        * @param outY Reference to out y coordinates
        */
        void GetCoords(float& outX, float& outY) const
        {
            outX = m_x;
            outY = m_y;
        }

        /**
        * @brief Get the current horizontal position
        * @return The x coordinate
        */
        float GetX() const
        {
            return m_x;
        }

        /**
        * @brief Get the current vertical position
        * @return The y coordinate
        */
        float GetY() const
        {
            return m_y;
        }

        /**
        * @brief Set the current rectangle coordinates
        * @param x The new x coordinates
        * @param y The new y coordinates
        */
        void SetCoords(float x, float y)
        {
            m_x = x;
            m_y = y;
            m_minX = x;
            m_minY = y;
            m_maxX = x + m_w;
            m_maxY = y + m_h;
        }

        /**
        * @brief Get the current rectangle dimensions
        * @param outW Reference to out width dimension
        * @param outH Reference to out height dimension
        */
        void GetDimensions(float& outW, float& outH) const { outW = m_w; outH = m_h; }

        /**
        * @brief Get the current width dimension
        * @return The width of the rectangle
        */
        float GetWidth() const
        {
            return m_w;
        }

        /**
        * @brief Get the current height dimension
        * @return The height of the rectangle
        */
        float GetHeight() const
        {
            return m_h;
        }

        /**
        * @brief Set the rectangle dimensions and update bounds
        * @param w The new width of the rectangle
        * @param h The new height of the rectangle
        */
        void SetDimensions(float w, float h)
        {
            m_w = w;
            m_h = h;
            m_maxX = m_x + w;
            m_maxY = m_y + h;
        }

        /**
        * @brief Direct helper to manually override full bounding ranges securely
        * @param minX The minimum horizontal position
        * @param minY The minimum vertical position
        * @param maxX The maximum horizontal position
        * @param maxY The maximum vertical position
        */
        void SetBounds(float minX, float minY, float maxX, float maxY)
        {
            m_minX = minX;
            m_minY = minY;
            m_maxX = maxX;
            m_maxY = maxY;
            m_x = minX;
            m_y = minY;
            m_w = maxX - minX;
            m_h = maxY - minY;
        }

        /**
        * @brief Checks if this rectangle overlaps with another axis-aligned rectangle
        * @param other Reference to the external rectangle to test against
        * @return True if there is a surface intersection, false otherwise
        */
        bool IsIntersecting(const Rectangle& other) const
        {
            // Get other rectangle min and max
            float otherMinX, otherMinY;
            float otherMaxX, otherMaxY;
            other.GetMin(otherMinX, otherMinY);
            other.GetMax(otherMaxX, otherMaxY);

            // Check separating axis theorem principles: if any edge gaps exist, they are completely separate
            bool intersecting = !(m_maxX < otherMinX || m_minX > otherMaxX || m_maxY < otherMinY || m_minY > otherMaxY);
            return intersecting;
        }

    private:
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_w = 0.0f;
        float m_h = 0.0f;

        float m_minX = 0.0f;
        float m_minY = 0.0f;
        float m_maxX = 0.0f;
        float m_maxY = 0.0f;
    };
}