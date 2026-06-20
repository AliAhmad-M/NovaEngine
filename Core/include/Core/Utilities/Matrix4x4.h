#pragma once
#include <cmath>
#include "Vector3.h"
#include "Vector4.h"

namespace Nova
{
    /**
     * 4x4 transformation matrix for 3D graphics
     *
     * Matrix layout (row-major):
     * | m11  m12  m13  m14 |
     * | m21  m22  m23  m24 |
     * | m31  m32  m33  m34 |
     * | m41  m42  m43  m44 |
     */
    struct Matrix4x4 {
        float m11, m12, m13, m14;
        float m21, m22, m23, m24;
        float m31, m32, m33, m34;
        float m41, m42, m43, m44;

        /**
         * Constructor - creates identity matrix by default
         */
        Matrix4x4(float m11 = 1, float m12 = 0, float m13 = 0, float m14 = 0,
            float m21 = 0, float m22 = 1, float m23 = 0, float m24 = 0,
            float m31 = 0, float m32 = 0, float m33 = 1, float m34 = 0,
            float m41 = 0, float m42 = 0, float m43 = 0, float m44 = 1)
            : m11(m11), m12(m12), m13(m13), m14(m14),
            m21(m21), m22(m22), m23(m23), m24(m24),
            m31(m31), m32(m32), m33(m33), m34(m34),
            m41(m41), m42(m42), m43(m43), m44(m44) {
        }

        static const Matrix4x4 IDENTITY;

        // Matrix operations
        /**
         * Matrix multiplication
         * @param other Matrix to multiply with
         * @return Result of this * other
         */
        Matrix4x4 operator*(const Matrix4x4& other) const {
            return {
                m11 * other.m11 + m12 * other.m21 + m13 * other.m31 + m14 * other.m41,
                m11 * other.m12 + m12 * other.m22 + m13 * other.m32 + m14 * other.m42,
                m11 * other.m13 + m12 * other.m23 + m13 * other.m33 + m14 * other.m43,
                m11 * other.m14 + m12 * other.m24 + m13 * other.m34 + m14 * other.m44,

                m21 * other.m11 + m22 * other.m21 + m23 * other.m31 + m24 * other.m41,
                m21 * other.m12 + m22 * other.m22 + m23 * other.m32 + m24 * other.m42,
                m21 * other.m13 + m22 * other.m23 + m23 * other.m33 + m24 * other.m43,
                m21 * other.m14 + m22 * other.m24 + m23 * other.m34 + m24 * other.m44,

                m31 * other.m11 + m32 * other.m21 + m33 * other.m31 + m34 * other.m41,
                m31 * other.m12 + m32 * other.m22 + m33 * other.m32 + m34 * other.m42,
                m31 * other.m13 + m32 * other.m23 + m33 * other.m33 + m34 * other.m43,
                m31 * other.m14 + m32 * other.m24 + m33 * other.m34 + m34 * other.m44,

                m41 * other.m11 + m42 * other.m21 + m43 * other.m31 + m44 * other.m41,
                m41 * other.m12 + m42 * other.m22 + m43 * other.m32 + m44 * other.m42,
                m41 * other.m13 + m42 * other.m23 + m43 * other.m33 + m44 * other.m43,
                m41 * other.m14 + m42 * other.m24 + m43 * other.m34 + m44 * other.m44
            };
        }

        /**
         * Multiply-assign operator
         */
        Matrix4x4& operator*=(const Matrix4x4& other) {
            *this = *this * other;
            return *this;
        }

        /**
         * Equality comparison
         */
        bool operator==(const Matrix4x4& other) const {
            return m11 == other.m11 && m12 == other.m12 && m13 == other.m13 && m14 == other.m14 &&
                m21 == other.m21 && m22 == other.m22 && m23 == other.m23 && m24 == other.m24 &&
                m31 == other.m31 && m32 == other.m32 && m33 == other.m33 && m34 == other.m34 &&
                m41 == other.m41 && m42 == other.m42 && m43 == other.m43 && m44 == other.m44;
        }

        /**
         * Inequality comparison
         */
        bool operator!=(const Matrix4x4& other) const {
            return !(*this == other);
        }

        // Static factory methods - Basic transformations
        /**
         * Creates an identity matrix
         */
        static Matrix4x4 Identity() {
            return Matrix4x4();
        }

        /**
         * Creates a translation matrix
         * @param position Translation vector
         */
        static Matrix4x4 CreateTranslation(const Vector3& position) {
            return {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                position.x, position.y, position.z, 1
            };
        }

        /**
         * Creates a translation matrix
         * @param x X translation
         * @param y Y translation
         * @param z Z translation
         */
        static Matrix4x4 CreateTranslation(float x, float y, float z) {
            return {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                x, y, z, 1
            };
        }

        /**
         * Creates a uniform scale matrix
         * @param scale Uniform scale factor
         */
        static Matrix4x4 CreateScale(float scale) {
            return {
                scale, 0, 0, 0,
                0, scale, 0, 0,
                0, 0, scale, 0,
                0, 0, 0, 1
            };
        }

        /**
         * Creates a non-uniform scale matrix
         * @param scaleX X scale factor
         * @param scaleY Y scale factor
         * @param scaleZ Z scale factor
         */
        static Matrix4x4 CreateScale(float scaleX, float scaleY, float scaleZ) {
            return {
                scaleX, 0, 0, 0,
                0, scaleY, 0, 0,
                0, 0, scaleZ, 0,
                0, 0, 0, 1
            };
        }

        /**
         * Creates a scale matrix from a vector
         * @param scale Scale vector
         */
        static Matrix4x4 CreateScale(const Vector3& scale) {
            return CreateScale(scale.x, scale.y, scale.z);
        }

        /**
         * Creates a rotation matrix around the X axis
         * @param radians Rotation angle in radians
         */
        static Matrix4x4 CreateRotationX(float radians) {
            float c = std::cos(radians);
            float s = std::sin(radians);
            return {
                1, 0, 0, 0,
                0, c, s, 0,
                0, -s, c, 0,
                0, 0, 0, 1
            };
        }

        /**
         * Creates a rotation matrix around the Y axis
         * @param radians Rotation angle in radians
         */
        static Matrix4x4 CreateRotationY(float radians) {
            float c = std::cos(radians);
            float s = std::sin(radians);
            return {
                c, 0, -s, 0,
                0, 1, 0, 0,
                s, 0, c, 0,
                0, 0, 0, 1
            };
        }

        /**
         * Creates a rotation matrix around the Z axis
         * @param radians Rotation angle in radians
         */
        static Matrix4x4 CreateRotationZ(float radians) {
            float c = std::cos(radians);
            float s = std::sin(radians);
            return {
                c, s, 0, 0,
                -s, c, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
        }

        /**
         * Creates a rotation matrix around an arbitrary axis
         * @param axis Axis to rotate around (should be normalized)
         * @param radians Rotation angle in radians
         */
        static Matrix4x4 CreateFromAxisAngle(const Vector3& axis, float radians) {
            float c = std::cos(radians);
            float s = std::sin(radians);
            float t = 1.0f - c;

            Vector3 a = axis.Normalize();

            return {
                t * a.x * a.x + c,          t * a.x * a.y + s * a.z,    t * a.x * a.z - s * a.y,    0,
                t * a.x * a.y - s * a.z,    t * a.y * a.y + c,          t * a.y * a.z + s * a.x,    0,
                t * a.x * a.z + s * a.y,    t * a.y * a.z - s * a.x,    t * a.z * a.z + c,          0,
                0,                          0,                          0,                          1
            };
        }

        /**
         * Creates a rotation matrix from Euler angles (pitch, yaw, roll)
         * @param pitch Rotation around X axis (radians)
         * @param yaw Rotation around Y axis (radians)
         * @param roll Rotation around Z axis (radians)
         */
        static Matrix4x4 CreateFromYawPitchRoll(float yaw, float pitch, float roll) {
            return CreateRotationY(yaw) * CreateRotationX(pitch) * CreateRotationZ(roll);
        }

        // View and Projection matrices
        /**
         * Creates a look-at view matrix (right-handed)
         * @param position Camera position
         * @param target Point camera is looking at
         * @param up Up direction (usually (0, 1, 0))
         */
        static Matrix4x4 CreateLookAt(const Vector3& position, const Vector3& target, const Vector3& up) {
            Vector3 zAxis = Vector3::Normalize(position - target);  // Forward
            Vector3 xAxis = Vector3::Normalize(Vector3::Cross(up, zAxis));  // Right
            Vector3 yAxis = Vector3::Cross(zAxis, xAxis);  // Up

            return {
                xAxis.x, yAxis.x, zAxis.x, 0,
                xAxis.y, yAxis.y, zAxis.y, 0,
                xAxis.z, yAxis.z, zAxis.z, 0,
                -Vector3::Dot(xAxis, position), -Vector3::Dot(yAxis, position), -Vector3::Dot(zAxis, position), 1
            };
        }

        /**
         * Creates a perspective projection matrix (right-handed)
         * @param fieldOfView Vertical field of view in radians
         * @param aspectRatio Aspect ratio (width / height)
         * @param nearPlane Near clipping plane distance
         * @param farPlane Far clipping plane distance
         */
        static Matrix4x4 CreatePerspectiveFieldOfView(float fieldOfView, float aspectRatio, float nearPlane, float farPlane) {
            float yScale = 1.0f / std::tan(fieldOfView * 0.5f);
            float xScale = yScale / aspectRatio;
            float range = farPlane / (nearPlane - farPlane);

            return {
                xScale, 0, 0, 0,
                0, yScale, 0, 0,
                0, 0, range, -1,
                0, 0, range * nearPlane, 0
            };
        }

        /**
         * Creates an orthographic projection matrix
         * @param width View width
         * @param height View height
         * @param nearPlane Near clipping plane
         * @param farPlane Far clipping plane
         */
        static Matrix4x4 CreateOrthographic(float width, float height, float nearPlane, float farPlane) {
            float range = 1.0f / (nearPlane - farPlane);

            return {
                2.0f / width, 0, 0, 0,
                0, 2.0f / height, 0, 0,
                0, 0, range, 0,
                0, 0, range * nearPlane, 1
            };
        }

        /**
         * Creates an orthographic projection matrix with explicit bounds
         * @param left Left boundary
         * @param right Right boundary
         * @param bottom Bottom boundary
         * @param top Top boundary
         * @param nearPlane Near clipping plane
         * @param farPlane Far clipping plane
         */
        static Matrix4x4 CreateOrthographicOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
            float width = right - left;
            float height = top - bottom;
            float depth = farPlane - nearPlane;

            return {
                2.0f / width, 0, 0, 0,
                0, 2.0f / height, 0, 0,
                0, 0, -2.0f / depth, 0,
                -(right + left) / width, -(top + bottom) / height, -(farPlane + nearPlane) / depth, 1
            };
        }

        // Transformation methods
        /**
         * Transforms a 3D point by this matrix
         * @param point Point to transform
         * @return Transformed point
         */
        Vector3 Transform(const Vector3& point) const {
            float w = point.x * m14 + point.y * m24 + point.z * m34 + m44;
            return {
                (point.x * m11 + point.y * m21 + point.z * m31 + m41) / w,
                (point.x * m12 + point.y * m22 + point.z * m32 + m42) / w,
                (point.x * m13 + point.y * m23 + point.z * m33 + m43) / w
            };
        }

        /**
         * Transforms a 4D vector by this matrix
         * @param vector Vector to transform
         * @return Transformed vector
         */
        Vector4 Transform(const Vector4& vector) const {
            return {
                vector.x * m11 + vector.y * m21 + vector.z * m31 + vector.w * m41,
                vector.x * m12 + vector.y * m22 + vector.z * m32 + vector.w * m42,
                vector.x * m13 + vector.y * m23 + vector.z * m33 + vector.w * m43,
                vector.x * m14 + vector.y * m24 + vector.z * m34 + vector.w * m44
            };
        }

        /**
         * Transforms a 3D vector (direction) by this matrix, ignoring translation
         * @param vector Vector to transform
         * @return Transformed vector
         */
        Vector3 TransformNormal(const Vector3& vector) const {
            return {
                vector.x * m11 + vector.y * m21 + vector.z * m31,
                vector.x * m12 + vector.y * m22 + vector.z * m32,
                vector.x * m13 + vector.y * m23 + vector.z * m33
            };
        }

        /**
         * Transposes the matrix
         * @return Transposed matrix
         */
        Matrix4x4 Transpose() const {
            return {
                m11, m21, m31, m41,
                m12, m22, m32, m42,
                m13, m23, m33, m43,
                m14, m24, m34, m44
            };
        }

        /**
         * Calculates the determinant of the matrix
         * @return Determinant value
         */
        float Determinant() const {
            float a = m11, b = m12, c = m13, d = m14;
            float e = m21, f = m22, g = m23, h = m24;
            float i = m31, j = m32, k = m33, l = m34;
            float m = m41, n = m42, o = m43, p = m44;

            float kp_lo = k * p - l * o;
            float jp_ln = j * p - l * n;
            float jo_kn = j * o - k * n;
            float ip_lm = i * p - l * m;
            float io_km = i * o - k * m;
            float in_jm = i * n - j * m;

            return a * (f * kp_lo - g * jp_ln + h * jo_kn) -
                b * (e * kp_lo - g * ip_lm + h * io_km) +
                c * (e * jp_ln - f * ip_lm + h * in_jm) -
                d * (e * jo_kn - f * io_km + g * in_jm);
        }

        /**
         * Inverts the matrix
         * @return Inverted matrix (or identity if not invertible)
         */
        Matrix4x4 Invert() const {
            float a = m11, b = m12, c = m13, d = m14;
            float e = m21, f = m22, g = m23, h = m24;
            float i = m31, j = m32, k = m33, l = m34;
            float m = m41, n = m42, o = m43, p = m44;

            float kp_lo = k * p - l * o;
            float jp_ln = j * p - l * n;
            float jo_kn = j * o - k * n;
            float ip_lm = i * p - l * m;
            float io_km = i * o - k * m;
            float in_jm = i * n - j * m;

            float a11 = +(f * kp_lo - g * jp_ln + h * jo_kn);
            float a12 = -(e * kp_lo - g * ip_lm + h * io_km);
            float a13 = +(e * jp_ln - f * ip_lm + h * in_jm);
            float a14 = -(e * jo_kn - f * io_km + g * in_jm);

            float det = a * a11 + b * a12 + c * a13 + d * a14;

            if (std::abs(det) < 1e-10f) {
                return Identity();
            }

            float invDet = 1.0f / det;

            float gp_ho = g * p - h * o;
            float fp_hn = f * p - h * n;
            float fo_gn = f * o - g * n;
            float ep_hm = e * p - h * m;
            float eo_gm = e * o - g * m;
            float en_fm = e * n - f * m;

            float gl_hk = g * l - h * k;
            float fl_hj = f * l - h * j;
            float fk_gj = f * k - g * j;
            float el_hi = e * l - h * i;
            float ek_gi = e * k - g * i;
            float ej_fi = e * j - f * i;

            return {
                a11 * invDet,
                -(b * kp_lo - c * jp_ln + d * jo_kn) * invDet,
                +(b * gp_ho - c * fp_hn + d * fo_gn) * invDet,
                -(b * gl_hk - c * fl_hj + d * fk_gj) * invDet,

                a12 * invDet,
                +(a * kp_lo - c * ip_lm + d * io_km) * invDet,
                -(a * gp_ho - c * ep_hm + d * eo_gm) * invDet,
                +(a * gl_hk - c * el_hi + d * ek_gi) * invDet,

                a13 * invDet,
                -(a * jp_ln - b * ip_lm + d * in_jm) * invDet,
                +(a * fp_hn - b * ep_hm + d * en_fm) * invDet,
                -(a * fl_hj - b * el_hi + d * ej_fi) * invDet,

                a14 * invDet,
                +(a * jo_kn - b * io_km + c * in_jm) * invDet,
                -(a * fo_gn - b * eo_gm + c * en_fm) * invDet,
                +(a * fk_gj - b * ek_gi + c * ej_fi) * invDet
            };
        }

        /**
         * Decomposes the matrix into scale, rotation, and translation
         * @param scale Output scale vector
         * @param rotation Output rotation matrix
         * @param translation Output translation vector
         * @return True if decomposition was successful
         */
        bool Decompose(Vector3& scale, Matrix4x4& rotation, Vector3& translation) const {
            translation = Vector3(m41, m42, m43);

            float xs = (m11 * m12 + m21 * m22 + m31 * m32) < 0 ? -1.0f : 1.0f;
            float ys = (m11 * m13 + m21 * m23 + m31 * m33) < 0 ? -1.0f : 1.0f;
            float zs = (m12 * m13 + m22 * m23 + m32 * m33) < 0 ? -1.0f : 1.0f;

            scale.x = xs * std::sqrt(m11 * m11 + m21 * m21 + m31 * m31);
            scale.y = ys * std::sqrt(m12 * m12 + m22 * m22 + m32 * m32);
            scale.z = zs * std::sqrt(m13 * m13 + m23 * m23 + m33 * m33);

            if (scale.x == 0.0f || scale.y == 0.0f || scale.z == 0.0f) {
                rotation = Identity();
                return false;
            }

            rotation = {
                m11 / scale.x, m12 / scale.y, m13 / scale.z, 0,
                m21 / scale.x, m22 / scale.y, m23 / scale.z, 0,
                m31 / scale.x, m32 / scale.y, m33 / scale.z, 0,
                0, 0, 0, 1
            };

            return true;
        }

        /**
         * Linear interpolation between two matrices
         * @param a Start matrix
         * @param b End matrix
         * @param t Interpolation factor (0 to 1)
         * @return Interpolated matrix
         */
        static Matrix4x4 Lerp(const Matrix4x4& a, const Matrix4x4& b, float t) {
            return {
                a.m11 + (b.m11 - a.m11) * t, a.m12 + (b.m12 - a.m12) * t, a.m13 + (b.m13 - a.m13) * t, a.m14 + (b.m14 - a.m14) * t,
                a.m21 + (b.m21 - a.m21) * t, a.m22 + (b.m22 - a.m22) * t, a.m23 + (b.m23 - a.m23) * t, a.m24 + (b.m24 - a.m24) * t,
                a.m31 + (b.m31 - a.m31) * t, a.m32 + (b.m32 - a.m32) * t, a.m33 + (b.m33 - a.m33) * t, a.m34 + (b.m34 - a.m34) * t,
                a.m41 + (b.m41 - a.m41) * t, a.m42 + (b.m42 - a.m42) * t, a.m43 + (b.m43 - a.m43) * t, a.m44 + (b.m44 - a.m44) * t
            };
        }

        /**
         * Gets the translation component
         * @return Translation vector
         */
        Vector3 GetTranslation() const {
            return Vector3(m41, m42, m43);
        }

        /**
         * Sets the translation component
         * @param translation New translation vector
         */
        void SetTranslation(const Vector3& translation) {
            m41 = translation.x;
            m42 = translation.y;
            m43 = translation.z;
        }
    };
}