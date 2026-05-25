#pragma once

#include <array>
#include <algorithm>
#include <limits>

#include <Core/Components/Transform2D.h>
#include <Core/Utilities/Matrix3x2.h>
#include <Core/Utilities/Rectangle.h>
#include <Core/Utilities/Vector2.h>
#include <Core/Utilities/Shape2D.h>
#include <Core/Utilities/Color.h>

#define M_PI 3.14159274f

/**
 * @brief Component that defines geometric shapes for collision detection and resolution.
 */
struct Collider2D
{
    // Offset of collider center from transform position
    Vector2 centerOffset;

    // Collision normal
    Vector2 normal = Vector2::ZERO;

    bool isColliding = false;
    bool isStatic;
    bool isSolid;

    /**
     * @brief Construct a new Circle Collider object.
     * @param centerOffset Offset from transform position.
     * @param r Radius of the circle.
     * @param isSolid Whether the collider resolves physical blockages.
     * @param isStatic Whether the object is completely stationary.
     */
    Collider2D(const Vector2& centerOffset, float r, bool isSolid = true, bool isStatic = false)
        : centerOffset(centerOffset), m_shape(Shape2D::CIRCLE), isSolid(isSolid), isStatic(isStatic)
    {
        // Initialize the collider AABB and dimensions
        m_aabb = Rectangle(-r, -r, r * 2.0f, r * 2.0f);
        m_circle.r = r;
    }

    /**
     * @brief Construct a new Box Collider object.
     * @param centerOffset Offset from transform position.
     * @param w Width of the box.
     * @param h Height of the box.
     * @param isSolid Whether the collider resolves physical blockages.
     * @param isStatic Whether the object is completely stationary.
     */
    Collider2D(const Vector2& centerOffset, float w, float h, bool isSolid = true, bool isStatic = false)
        : centerOffset(centerOffset), m_shape(Shape2D::BOX), isSolid(isSolid), isStatic(isStatic)
    {
        // Initialize the collider AABB and dimensions
        m_aabb = Rectangle(-w / 2.0f, -h / 2.0f, w, h);
        m_box.w = w;
        m_box.h = h;

        // Initialize box vertices
        m_box.vertices[0] = Vector2(-w / 2, -h / 2); // Bottom left
        m_box.vertices[1] = Vector2(w / 2, -h / 2); // Bottom right  
        m_box.vertices[2] = Vector2(w / 2, h / 2); // Top right
        m_box.vertices[3] = Vector2(-w / 2, h / 2); // Top left

        m_box.transformedVertices = m_box.vertices;
    }

    /**
     * @brief Computes and updates world-space vertices based on a transform.
     * @param transform Source transformation data.
     * @return Reference to the array of 4 transformed vertices.
     */
    std::array<Vector2, 4>& GetTransformedVertices(const Transform2D* transform)
    {
        // Return if no need to update vertices
        if (m_shape == Shape2D::CIRCLE)
        {
            static std::array<Vector2, 4> empty{};
            return empty;
        }

        float radians = transform->rotation * ((float)M_PI / 180.0f);

        // Create a transformation matrix
        Matrix3x2 transformationMatrix =
            Matrix3x2::CreateScale(transform->scale.x, transform->scale.y) *
            Matrix3x2::CreateRotation(radians) *
            Matrix3x2::CreateTranslation(
                Vector2(
                    centerOffset.x * transform->scale.x,
                    centerOffset.y * transform->scale.y)
                + transform->position
            );

        // Update transformed vertices using the transformation matrix
        for (int i = 0; i < m_box.vertices.size(); i++)
        {
            m_box.transformedVertices[i] = transformationMatrix.Transform(m_box.vertices[i]);
        }

        return m_box.transformedVertices;
    }

    /**
     * @brief Computes and returns the updated Axis-Aligned Bounding Box.
     * @param transform Source transformation data.
     * @return Pointer to the updated internal AABB tracker.
     */
    Rectangle* GetAABB(const Transform2D* transform)
    {
        if (m_shape == Shape2D::BOX)
        {
            const auto& vertices = GetTransformedVertices(transform);

            // Find min and max position of edges using vertices
            float minX = std::numeric_limits<float>::infinity();
            float minY = std::numeric_limits<float>::infinity();
            float maxX = -std::numeric_limits<float>::infinity();
            float maxY = -std::numeric_limits<float>::infinity();

            for (const auto& vertex : vertices)
            {
                minX = std::min(minX, vertex.x);
                minY = std::min(minY, vertex.y);
                maxX = std::max(maxX, vertex.x);
                maxY = std::max(maxY, vertex.y);
            }

            // Update the AABB bounds safely via encapsulation helper
            m_aabb.SetBounds(minX, minY, maxX, maxY);
        }
        else
        {
            Vector2 worldCenter = Vector2(
                centerOffset.x * transform->scale.x,
                centerOffset.y * transform->scale.y) + transform->position;

            float minX = worldCenter.x - m_circle.r * transform->scale.x;
            float minY = worldCenter.y - m_circle.r * transform->scale.y;
            float maxX = worldCenter.x + m_circle.r * transform->scale.x;
            float maxY = worldCenter.y + m_circle.r * transform->scale.y;

            m_aabb.SetBounds(minX, minY, maxX, maxY);
        }

        return &m_aabb;
    }

    /**
     * @brief Calculates the center point of the collider in world space.
     * @param transform Source transformation data.
     * @return The absolute coordinates of the center.
     */
    Vector2 GetGlobalCenter(const Transform2D* transform) const
    {
        Vector2 globalCenter = Vector2(
            centerOffset.x * transform->scale.x,
            centerOffset.y * transform->scale.y) + transform->position;

        return globalCenter;
    }

    /**
     * @brief Get the primitive shape type of this collider.
     * @return Shape2D variant identifier enum.
     */
    Shape2D GetShape() const
    {
        return m_shape;
    }

    /**
    * @brief Gets the scaled radius of a circle collider.
    * @param scaleX Manual horizontal sizing multiplier factor.
    * @param scaleY Manual vertical sizing multiplier factor.
    * @return Resulting runtime circle radius.
    */
    float GetRadius(float scaleX, float scaleY) const
    {
        return m_circle.r * std::max(scaleX, scaleY);
    }

    /**
     * @brief Gets the scaled width of a box collider.
     * @param scaleX Manual horizontal sizing multiplier factor.
     * @return Resulting runtime box width.
     */
    float GetWidth(float scaleX) const
    {
        return m_box.w * scaleX;
    }

    /**
     * @brief Gets the scaled height of a box collider.
     * @param scaleY Manual vertical sizing multiplier factor.
     * @return Resulting runtime box height.
     */
    float GetHeight(float scaleY) const
    {
        return m_box.h * scaleY;
    }

    /**
     * @brief Checks if contact normal indicates surface grounding below.
     */
    bool IsOnFloor() const
    {
        return normal.y > 0.5f;
    }

    /**
     * @brief Checks if contact normal indicates collision on left face.
     */
    bool IsOnLeftWall() const
    {
        return normal.x > 0.5f;
    }

    /**
     * @brief Checks if contact normal indicates collision on right face.
     */
    bool IsOnRightWall() const
    {
        return normal.x < -0.5f;
    }

    /**
     * @brief Checks if contact normal indicates ceiling surface overhead.
     */
    bool IsOnCeiling() const
    {
        return normal.y < -0.5f;
    }

private:
    // Shape type
    Shape2D m_shape;

    // Bounding box
    Rectangle m_aabb;

    // Shape variants
    struct {
        float r = 0.0f;
    } m_circle;

    struct {
        float w = 0.0f;
        float h = 0.0f;
        std::array<Vector2, 4> vertices;
        std::array<Vector2, 4> transformedVertices;
    } m_box;
};