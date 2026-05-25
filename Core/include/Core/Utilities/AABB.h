#pragma once

struct AABB
{
    // Boundaries

    float minX;
    float minY;

    float maxX;
    float maxY;

    /**
     * Constructor
     * @param w AABB width
     * @param h AABB height
     */
    AABB(float w = 0, float h = 0) : m_isIntersecting(false)
    {
        minX = -w / 2;
        minY = -h / 2;
        
        maxX =  w / 2;
        maxY =  h / 2;
    }

    /**
     * Checks for AABB intersection
     * @param other Reference to other AABB instance
     */
    bool IsIntersecting(AABB& other)
    {
        // Set isColliding to true if there is overlap on both the X and Y axes
        m_isIntersecting = !(maxX < other.minX || minX> other.maxX || maxY < other.minY || minY > other.maxY);
        other.m_isIntersecting = m_isIntersecting;

        return m_isIntersecting;
    }

private:
    bool m_isIntersecting;
};