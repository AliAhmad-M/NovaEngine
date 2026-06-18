#pragma once
#include <Core/Utilities/Vector2.h>
#include <array>

struct Camera2D
{
	bool isMainCamera;

    Camera2D(bool isMainCamera = false, 
		float m_zoom = 1.0f, 
		std::array<float, 4> m_boundaries = { -INFINITY, INFINITY, -INFINITY, INFINITY })
    {
		SetZoom(m_zoom);
		SetBoundaries(m_boundaries);
    }

	float GetZoom() const
	{
		return m_zoom;
	}

	float SetZoom(float newZoom)
	{
		m_zoom = std::max(0.01f, newZoom);
		return m_zoom;
	}

	std::array<float, 4> GetBoundaries() const
	{
		return m_boundaries;
	}

	void SetBoundaries(std::array<float, 4> newBoundaries)
	{
		float left	= newBoundaries[0];
		float right = newBoundaries[1];
		float up	= newBoundaries[2];
		float down	= newBoundaries[3];

		float boundaryWidth = right - left;
		float boundaryHeight = down - up;

		// Make sure boundaries are not less than viewport size
		if (boundaryWidth < m_width)
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

	const Vector2& GetOffset() const
	{
		return m_offset;
	}

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

	void GetViewPortSize(int& width, int& height) const
	{
		width = m_width;
		height = m_height;
	}

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