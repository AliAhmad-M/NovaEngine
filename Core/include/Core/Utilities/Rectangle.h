#pragma once

/**
 * @brief Represents a 2D axis-aligned rectangle.
 */
struct Rectangle {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

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
        : x(x), y(y), w(w), h(h) {
    }
};