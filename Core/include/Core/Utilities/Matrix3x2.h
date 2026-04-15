#pragma once
#pragma once
#include <cmath>
#include "Vector2.h"

/**
 * 3x2 transformation matrix for 2D graphics
 *
 * Matrix layout:
 * | m11  m12 |
 * | m21  m22 |
 * | m31  m32 |
 */
struct Matrix3x2 {
    float m11, m12;
    float m21, m22;
    float m31, m32;

    /**
     * Constructor
     * @param m11 Element at row 1, column 1 (default 1)
     * @param m12 Element at row 1, column 2 (default 0)
     * @param m21 Element at row 2, column 1 (default 0)
     * @param m22 Element at row 2, column 2 (default 1)
     * @param m31 Element at row 3, column 1 - X translation (default 0)
     * @param m32 Element at row 3, column 2 - Y translation (default 0)
     */
    Matrix3x2(float m11 = 1, float m12 = 0,
        float m21 = 0, float m22 = 1,
        float m31 = 0, float m32 = 0)
        : m11(m11), m12(m12), m21(m21), m22(m22), m31(m31), m32(m32) {
    }

    static const Matrix3x2 IDENTITY; // Identity matrix

    // Matrix operations
    /**
     * Matrix multiplication
     * @param other Matrix to multiply with
     * @return Result of this * other
     */
    Matrix3x2 operator*(const Matrix3x2& other) const {
        return {
            m11 * other.m11 + m12 * other.m21,
            m11 * other.m12 + m12 * other.m22,
            m21 * other.m11 + m22 * other.m21,
            m21 * other.m12 + m22 * other.m22,
            m31 * other.m11 + m32 * other.m21 + other.m31,
            m31 * other.m12 + m32 * other.m22 + other.m32
        };
    }

    /**
     * Multiply-assign operator
     * @param other Matrix to multiply with
     * @return Reference to this matrix after multiplication
     */
    Matrix3x2& operator*=(const Matrix3x2& other) {
        *this = *this * other;
        return *this;
    }

    /**
     * Equality comparison
     * @param other Matrix to compare with
     * @return True if all elements are equal
     */
    bool operator==(const Matrix3x2& other) const {
        return m11 == other.m11 && m12 == other.m12 &&
            m21 == other.m21 && m22 == other.m22 &&
            m31 == other.m31 && m32 == other.m32;
    }

    /**
     * Inequality comparison
     * @param other Matrix to compare with
     * @return True if any element is different
     */
    bool operator!=(const Matrix3x2& other) const {
        return !(*this == other);
    }

    // Static factory methods
    /**
     * Creates a rotation matrix
     * @param rotation Rotation angle in radians (clockwise)
     * @return Rotation matrix
     */
    static Matrix3x2 CreateRotation(float rotation) {
        float cos = std::cos(rotation);
        float sin = std::sin(rotation);
        return { cos, sin, -sin, cos, 0, 0 };
    }

    /**
     * Creates a rotation matrix around a specific point
     * @param rotation Rotation angle in radians
     * @param center Center point of rotation
     * @return Rotation matrix
     */
    static Matrix3x2 CreateRotation(float rotation, const Vector2& center) {
        float cos = std::cos(rotation);
        float sin = std::sin(rotation);
        float tx = center.x * (1 - cos) + center.y * sin;
        float ty = center.y * (1 - cos) - center.x * sin;
        return { cos, sin, -sin, cos, tx, ty };
    }

    /**
     * Creates a translation matrix
     * @param position Translation offset
     * @return Translation matrix
     */
    static Matrix3x2 CreateTranslation(const Vector2& position) {
        return { 1, 0, 0, 1, position.x, position.y };
    }

    /**
     * Creates a translation matrix
     * @param x X translation
     * @param y Y translation
     * @return Translation matrix
     */
    static Matrix3x2 CreateTranslation(float x, float y) {
        return { 1, 0, 0, 1, x, y };
    }

    /**
     * Creates a uniform scale matrix
     * @param scale Uniform scale factor
     * @return Scale matrix
     */
    static Matrix3x2 CreateScale(float scale) {
        return { scale, 0, 0, scale, 0, 0 };
    }

    /**
     * Creates a non-uniform scale matrix
     * @param scaleX Scale factor for X axis
     * @param scaleY Scale factor for Y axis
     * @return Scale matrix
     */
    static Matrix3x2 CreateScale(float scaleX, float scaleY) {
        return { scaleX, 0, 0, scaleY, 0, 0 };
    }

    /**
     * Creates a scale matrix with a center point
     * @param scale Uniform scale factor
     * @param center Center point of scaling
     * @return Scale matrix
     */
    static Matrix3x2 CreateScale(float scale, const Vector2& center) {
        float tx = center.x * (1 - scale);
        float ty = center.y * (1 - scale);
        return { scale, 0, 0, scale, tx, ty };
    }

    /**
     * Creates a scale matrix with a center point
     * @param scaleX Scale factor for X axis
     * @param scaleY Scale factor for Y axis
     * @param center Center point of scaling
     * @return Scale matrix
     */
    static Matrix3x2 CreateScale(float scaleX, float scaleY, const Vector2& center) {
        float tx = center.x * (1 - scaleX);
        float ty = center.y * (1 - scaleY);
        return { scaleX, 0, 0, scaleY, tx, ty };
    }

    /**
     * Creates a skew matrix
     * @param radiansX Skew angle for X axis in radians
     * @param radiansY Skew angle for Y axis in radians
     * @return Skew matrix
     */
    static Matrix3x2 CreateSkew(float radiansX, float radiansY) {
        float tanX = std::tan(radiansX);
        float tanY = std::tan(radiansY);
        return { 1, tanY, tanX, 1, 0, 0 };
    }

    /**
     * Creates an identity matrix
     * @return Identity matrix
     */
    static Matrix3x2 Identity() {
        return { 1, 0, 0, 1, 0, 0 };
    }

    // Transformation methods
    /**
     * Transforms a point by this matrix
     * @param point Point to transform
     * @return Transformed point
     */
    Vector2 Transform(const Vector2& point) const {
        return {
            point.x * m11 + point.y * m21 + m31,
            point.x * m12 + point.y * m22 + m32
        };
    }

    /**
     * Transforms a vector by this matrix (ignores translation)
     * @param vector Vector to transform
     * @return Transformed vector
     */
    Vector2 TransformVector(const Vector2& vector) const {
        return {
            vector.x * m11 + vector.y * m21,
            vector.x * m12 + vector.y * m22
        };
    }

    /**
     * Calculates the determinant of the matrix
     * @return Determinant value
     */
    float Determinant() const {
        return m11 * m22 - m12 * m21;
    }

    /**
     * Checks if this matrix is invertible
     * @return True if determinant is non-zero
     */
    bool IsInvertible() const {
        return Determinant() != 0;
    }

    /**
     * Inverts the matrix
     * @return Inverted matrix (or identity if not invertible)
     */
    Matrix3x2 Invert() const {
        float det = Determinant();
        if (det == 0) {
            return Identity();
        }

        float invDet = 1.0f / det;
        return {
             m22 * invDet,
            -m12 * invDet,
            -m21 * invDet,
             m11 * invDet,
            (m21 * m32 - m22 * m31) * invDet,
            (m12 * m31 - m11 * m32) * invDet
        };
    }

    /**
     * Gets the translation component
     * @return Translation vector
     */
    Vector2 GetTranslation() const {
        return { m31, m32 };
    }

    /**
     * Sets the translation component
     * @param translation New translation vector
     */
    void SetTranslation(const Vector2& translation) {
        m31 = translation.x;
        m32 = translation.y;
    }

    /**
     * Linear interpolation between two matrices
     * @param a Start matrix
     * @param b End matrix
     * @param t Interpolation factor (0 to 1)
     * @return Interpolated matrix
     */
    static Matrix3x2 Lerp(const Matrix3x2& a, const Matrix3x2& b, float t) {
        return {
            a.m11 + (b.m11 - a.m11) * t,
            a.m12 + (b.m12 - a.m12) * t,
            a.m21 + (b.m21 - a.m21) * t,
            a.m22 + (b.m22 - a.m22) * t,
            a.m31 + (b.m31 - a.m31) * t,
            a.m32 + (b.m32 - a.m32) * t
        };
    }
};