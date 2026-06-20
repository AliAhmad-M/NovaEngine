#pragma once
#include <cmath>
#include "Vector2.h"
#include "Vector3.h"

namespace Nova 
{
    /**
     * 3x3 matrix
     *
     * Matrix layout:
     * | m11  m12  m13 |
     * | m21  m22  m23 |
     * | m31  m32  m33 |
     */
    struct Matrix3x3 
    {
        float m11, m12, m13;
        float m21, m22, m23;
        float m31, m32, m33;

        /**
         * Constructor, creates identity matrix by default
         */
        Matrix3x3(float m11 = 1, float m12 = 0, float m13 = 0,
            float m21 = 0, float m22 = 1, float m23 = 0,
            float m31 = 0, float m32 = 0, float m33 = 1)
            : m11(m11), m12(m12), m13(m13),
            m21(m21), m22(m22), m23(m23),
            m31(m31), m32(m32), m33(m33) {}

        static const Matrix3x3 IDENTITY;

        /**
         * Matrix multiplication
         * @param other Matrix to multiply with
         * @return Result of this * other
         */
        Matrix3x3 operator*(const Matrix3x3& other) const 
        {
            return {
                m11 * other.m11 + m12 * other.m21 + m13 * other.m31,
                m11 * other.m12 + m12 * other.m22 + m13 * other.m32,
                m11 * other.m13 + m12 * other.m23 + m13 * other.m33,

                m21 * other.m11 + m22 * other.m21 + m23 * other.m31,
                m21 * other.m12 + m22 * other.m22 + m23 * other.m32,
                m21 * other.m13 + m22 * other.m23 + m23 * other.m33,

                m31 * other.m11 + m32 * other.m21 + m33 * other.m31,
                m31 * other.m12 + m32 * other.m22 + m33 * other.m32,
                m31 * other.m13 + m32 * other.m23 + m33 * other.m33
            };
        }

        /**
         * Multiply-assign operator
         */
        Matrix3x3& operator*=(const Matrix3x3& other) 
        {
            *this = *this * other;
            return *this;
        }

        /**
         * Scalar multiplication
         */
        Matrix3x3 operator*(float scalar) const 
        {
            return 
            {
                m11 * scalar, m12 * scalar, m13 * scalar,
                m21 * scalar, m22 * scalar, m23 * scalar,
                m31 * scalar, m32 * scalar, m33 * scalar
            };
        }

        /**
         * Matrix addition
         */
        Matrix3x3 operator+(const Matrix3x3& other) const 
        {
            return 
            {
                m11 + other.m11, m12 + other.m12, m13 + other.m13,
                m21 + other.m21, m22 + other.m22, m23 + other.m23,
                m31 + other.m31, m32 + other.m32, m33 + other.m33
            };
        }

        /**
         * Matrix subtraction
         */
        Matrix3x3 operator-(const Matrix3x3& other) const 
        {
            return 
            {
                m11 - other.m11, m12 - other.m12, m13 - other.m13,
                m21 - other.m21, m22 - other.m22, m23 - other.m23,
                m31 - other.m31, m32 - other.m32, m33 - other.m33
            };
        }

        /**
         * Equality comparison
         */
        bool operator==(const Matrix3x3& other) const 
        {
            return m11 == other.m11 && m12 == other.m12 && m13 == other.m13 &&
                m21 == other.m21 && m22 == other.m22 && m23 == other.m23 &&
                m31 == other.m31 && m32 == other.m32 && m33 == other.m33;
        }

        /**
         * Inequality comparison
         */
        bool operator!=(const Matrix3x3& other) const 
        {
            return !(*this == other);
        }

        // Static factory methods
        /**
         * Creates an identity matrix
         */
        static Matrix3x3 Identity() 
        {
            return Matrix3x3();
        }

        /**
         * Creates a 2D translation matrix (homogeneous coordinates)
         * @param x X translation
         * @param y Y translation
         */
        static Matrix3x3 CreateTranslation(float x, float y) 
        {
            return 
            {
                1, 0, 0,
                0, 1, 0,
                x, y, 1
            };
        }

        /**
         * Creates a 2D translation matrix (homogeneous coordinates)
         * @param position Translation vector
         */
        static Matrix3x3 CreateTranslation(const Vector2& position) 
        {
            return CreateTranslation(position.x, position.y);
        }

        /**
         * Creates a 2D rotation matrix
         * @param radians Rotation angle in radians
         */
        static Matrix3x3 CreateRotation(float radians) 
        {
            float c = std::cos(radians);
            float s = std::sin(radians);
            return 
            {
                c, s, 0,
                -s, c, 0,
                0, 0, 1
            };
        }

        /**
         * Creates a 2D scale matrix
         * @param scaleX X scale factor
         * @param scaleY Y scale factor
         */
        static Matrix3x3 CreateScale(float scaleX, float scaleY) 
        {
            return 
            {
                scaleX, 0, 0,
                0, scaleY, 0,
                0, 0, 1
            };
        }

        /**
         * Creates a uniform 2D scale matrix
         * @param scale Uniform scale factor
         */
        static Matrix3x3 CreateScale(float scale)
        {
            return CreateScale(scale, scale);
        }

        /**
         * Creates a 3D rotation matrix around X axis
         * @param radians Rotation angle in radians
         */
        static Matrix3x3 CreateRotationX(float radians) 
        {
            float c = std::cos(radians);
            float s = std::sin(radians);
            return 
            {
                1, 0, 0,
                0, c, s,
                0, -s, c
            };
        }

        /**
         * Creates a 3D rotation matrix around Y axis
         * @param radians Rotation angle in radians
         */
        static Matrix3x3 CreateRotationY(float radians) 
        {
            float c = std::cos(radians);
            float s = std::sin(radians);
            return 
            {
                c, 0, -s,
                0, 1, 0,
                s, 0, c
            };
        }

        /**
         * Creates a 3D rotation matrix around Z axis
         * @param radians Rotation angle in radians
         */
        static Matrix3x3 CreateRotationZ(float radians) 
        {
            float c = std::cos(radians);
            float s = std::sin(radians);
            return 
            {
                c, s, 0,
                -s, c, 0,
                0, 0, 1
            };
        }

        /**
         * Creates a 3D rotation matrix from an axis and angle
         * @param axis Rotation axis (should be normalized)
         * @param radians Rotation angle in radians
         */
        static Matrix3x3 CreateFromAxisAngle(const Vector3& axis, float radians) 
        {
            float c = std::cos(radians);
            float s = std::sin(radians);
            float t = 1.0f - c;

            Vector3 a = axis.Normalize();

            return 
            {
                t * a.x * a.x + c,          t * a.x * a.y + s * a.z,    t * a.x * a.z - s * a.y,
                t * a.x * a.y - s * a.z,    t * a.y * a.y + c,          t * a.y * a.z + s * a.x,
                t * a.x * a.z + s * a.y,    t * a.y * a.z - s * a.x,    t * a.z * a.z + c
            };
        }

        // Transformation methods
        /**
         * Transforms a 2D point in homogeneous coordinates
         * @param point Point to transform
         * @return Transformed point
         */
        Vector2 Transform(const Vector2& point) const 
        {
            float w = point.x * m13 + point.y * m23 + m33;
            return 
            {
                (point.x * m11 + point.y * m21 + m31) / w,
                (point.x * m12 + point.y * m22 + m32) / w
            };
        }

        /**
         * Transforms a 3D vector
         * @param vector Vector to transform
         * @return Transformed vector
         */
        Vector3 Transform(const Vector3& vector) const 
        {
            return 
            {
                vector.x * m11 + vector.y * m21 + vector.z * m31,
                vector.x * m12 + vector.y * m22 + vector.z * m32,
                vector.x * m13 + vector.y * m23 + vector.z * m33
            };
        }

        /**
         * Calculates the determinant
         * @return Determinant value
         */
        float Determinant() const 
        {
            return m11 * (m22 * m33 - m23 * m32) -
                m12 * (m21 * m33 - m23 * m31) +
                m13 * (m21 * m32 - m22 * m31);
        }

        /**
         * Transposes the matrix
         * @return Transposed matrix
         */
        Matrix3x3 Transpose() const 
        {
            return 
            {
                m11, m21, m31,
                m12, m22, m32,
                m13, m23, m33
            };
        }

        /**
         * Inverts the matrix
         * @return Inverted matrix (or identity if not invertible)
         */
        Matrix3x3 Invert() const 
        {
            float det = Determinant();
            if (std::abs(det) < 1e-10f) 
            {
                return Identity();
            }

            float invDet = 1.0f / det;

            return {
                (m22 * m33 - m23 * m32) * invDet,
                (m13 * m32 - m12 * m33) * invDet,
                (m12 * m23 - m13 * m22) * invDet,

                (m23 * m31 - m21 * m33) * invDet,
                (m11 * m33 - m13 * m31) * invDet,
                (m13 * m21 - m11 * m23) * invDet,

                (m21 * m32 - m22 * m31) * invDet,
                (m12 * m31 - m11 * m32) * invDet,
                (m11 * m22 - m12 * m21) * invDet
            };
        }

        /**
         * Creates a matrix from basis vectors
         * @param xAxis X-axis basis vector
         * @param yAxis Y-axis basis vector
         * @param zAxis Z-axis basis vector
         * @return Matrix with the given basis
         */
        static Matrix3x3 CreateFromBasis(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis) 
        {
            return 
            {
                xAxis.x, xAxis.y, xAxis.z,
                yAxis.x, yAxis.y, yAxis.z,
                zAxis.x, zAxis.y, zAxis.z
            };
        }

        /**
         * Linear interpolation between two matrices
         * @param a Start matrix
         * @param b End matrix
         * @param t Interpolation factor (0 to 1)
         * @return Interpolated matrix
         */
        static Matrix3x3 Lerp(const Matrix3x3& a, const Matrix3x3& b, float t) 
        {
            return 
            {
                a.m11 + (b.m11 - a.m11) * t, a.m12 + (b.m12 - a.m12) * t, a.m13 + (b.m13 - a.m13) * t,
                a.m21 + (b.m21 - a.m21) * t, a.m22 + (b.m22 - a.m22) * t, a.m23 + (b.m23 - a.m23) * t,
                a.m31 + (b.m31 - a.m31) * t, a.m32 + (b.m32 - a.m32) * t, a.m33 + (b.m33 - a.m33) * t
            };
        }

        /**
         * Gets the scale from the matrix
         * @return Scale vector
         */
        Vector3 GetScale() const 
        {
            return 
            {
                std::sqrt(m11 * m11 + m12 * m12 + m13 * m13),
                std::sqrt(m21 * m21 + m22 * m22 + m23 * m23),
                std::sqrt(m31 * m31 + m32 * m32 + m33 * m33)
            };
        }
    };
}
