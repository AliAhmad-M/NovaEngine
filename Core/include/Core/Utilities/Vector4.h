#pragma once
#include <cmath>

namespace Nova 
{
    /**
     * 4D vector with floating point components
     */
    struct Vector4 {
        float x;
        float y;
        float z;
        float w;

        static const Vector4 ZERO;   // ( 0, 0, 0, 0 )
        static const Vector4 ONE;    // ( 1, 1, 1, 1 )
        static const Vector4 UNIT_X; // ( 1, 0, 0, 0 )
        static const Vector4 UNIT_Y; // ( 0, 1, 0, 0 )
        static const Vector4 UNIT_Z; // ( 0, 0, 1, 0 )
        static const Vector4 UNIT_W; // ( 0, 0, 0, 1 )

        /**
         * Constructor
         * @param x X component (default 0)
         * @param y Y component (default 0)
         * @param z Z component (default 0)
         * @param w W component (default 0)
         */
        Vector4(float x = 0, float y = 0, float z = 0, float w = 0) : x(x), y(y), z(z), w(w) {}

        // Arithmetic operation overloading
        Vector4 operator+(const Vector4& b) const {
            return { x + b.x, y + b.y, z + b.z, w + b.w };
        }

        Vector4 operator-(const Vector4& b) const {
            return { x - b.x, y - b.y, z - b.z, w - b.w };
        }

        Vector4 operator*(float s) const {
            return { x * s, y * s, z * s, w * s };
        }

        Vector4 operator/(float s) const {
            return { x / s, y / s, z / s, w / s };
        }

        Vector4 operator-() const {
            return { -x, -y, -z, -w };
        }

        Vector4& operator+=(const Vector4& b) {
            x += b.x;
            y += b.y;
            z += b.z;
            w += b.w;
            return *this;
        }

        Vector4& operator-=(const Vector4& b) {
            x -= b.x;
            y -= b.y;
            z -= b.z;
            w -= b.w;
            return *this;
        }

        Vector4& operator*=(float s) {
            x *= s;
            y *= s;
            z *= s;
            w *= s;
            return *this;
        }

        Vector4& operator/=(float s) {
            x /= s;
            y /= s;
            z /= s;
            w /= s;
            return *this;
        }

        // Comparison operator overloading
        bool operator==(const Vector4& b) const {
            return x == b.x && y == b.y && z == b.z && w == b.w;
        }

        bool operator!=(const Vector4& b) const {
            return !(*this == b);
        }

        bool operator>(const Vector4& b) const {
            return MagnitudeSquared() > b.MagnitudeSquared();
        }

        bool operator>=(const Vector4& b) const {
            return MagnitudeSquared() >= b.MagnitudeSquared();
        }

        bool operator<(const Vector4& b) const {
            return MagnitudeSquared() < b.MagnitudeSquared();
        }

        bool operator<=(const Vector4& b) const {
            return MagnitudeSquared() <= b.MagnitudeSquared();
        }

        // Static methods
        /**
         * Adds two vectors
         * @param a First vector
         * @param b Second vector
         * @return Vector (a + b)
         */
        static Vector4 Add(const Vector4& a, const Vector4& b) {
            return a + b;
        }

        /**
         * Subtracts two vectors
         * @param a First vector
         * @param b Second vector
         * @return Vector (a - b)
         */
        static Vector4 Subtract(const Vector4& a, const Vector4& b) {
            return a - b;
        }

        /**
         * Multiplies vector by scalar
         * @param a Vector
         * @param s Scalar value
         * @return Vector (a * s)
         */
        static Vector4 Multiply(const Vector4& a, float s) {
            return a * s;
        }

        /**
         * Divides vector by scalar
         * @param a Vector
         * @param s Scalar value (non-zero)
         * @return Vector (a / s)
         */
        static Vector4 Divide(const Vector4& a, float s) {
            return a / s;
        }

        /**
         * Negates a vector
         * @param a Vector
         * @return Vector (-a)
         */
        static Vector4 Negate(const Vector4& a) {
            return -a;
        }

        /**
         * Dot product of two vectors
         * @param a First vector
         * @param b Second vector
         * @return Dot product (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w)
         */
        static float Dot(const Vector4& a, const Vector4& b) {
            return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        }

        /**
         * Squared magnitude of a vector
         * @param a Vector
         * @return a.x² + a.y² + a.z² + a.w²
         */
        static float MagnitudeSquared(const Vector4& a) {
            return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
        }

        /**
         * Magnitude (length) of a vector
         * @param a Vector
         * @return sqrt(a.x² + a.y² + a.z² + a.w²)
         */
        static float Magnitude(const Vector4& a) {
            return std::sqrt(MagnitudeSquared(a));
        }

        /**
         * Normalized unit vector
         * @param a Vector
         * @return Vector with same direction, length 1 (or zero vector if length is 0)
         */
        static Vector4 Normalize(const Vector4& a) {
            float len = Magnitude(a);
            return len == 0 ? Vector4(0, 0, 0, 0) : a / len;
        }

        /**
         * Linear interpolation between two vectors
         * @param a Start vector
         * @param b End vector
         * @param t Interpolation factor (0 to 1)
         * @return Interpolated vector
         */
        static Vector4 Lerp(const Vector4& a, const Vector4& b, float t) {
            return a + (b - a) * t;
        }

        /**
         * Distance between two points
         * @param a First point
         * @param b Second point
         * @return Distance between a and b
         */
        static float Distance(const Vector4& a, const Vector4& b) {
            return Magnitude(b - a);
        }

        /**
         * Squared distance between two points
         * @param a First point
         * @param b Second point
         * @return Squared distance between a and b
         */
        static float DistanceSquared(const Vector4& a, const Vector4& b) {
            return MagnitudeSquared(b - a);
        }

        /**
         * Checks if two vectors are equal
         * @param a First vector
         * @param b Second vector
         * @return True if all components are equal
         */
        static bool Equals(const Vector4& a, const Vector4& b) {
            return a == b;
        }

        // Instance methods
        /**
         * Adds another vector to this vector
         * @param b Vector to add
         * @return New vector as (*this + b)
         */
        Vector4 Add(const Vector4& b) const {
            return *this + b;
        }

        /**
         * Subtracts another vector from this vector
         * @param b Vector to subtract
         * @return New vector as (*this - b)
         */
        Vector4 Subtract(const Vector4& b) const {
            return *this - b;
        }

        /**
         * Multiplies this vector by a scalar
         * @param s Scalar value
         * @return New vector as (*this * s)
         */
        Vector4 Multiply(float s) const {
            return *this * s;
        }

        /**
         * Divides this vector by a scalar
         * @param s Scalar value (non-zero)
         * @return New vector as (*this / s)
         */
        Vector4 Divide(float s) const {
            return *this / s;
        }

        /**
         * Negates this vector
         * @return New vector as (-*this)
         */
        Vector4 Negate() const {
            return -*this;
        }

        /**
         * Dot product with another vector
         * @param b Other vector
         * @return Dot product (x * b.x + y * b.y + z * b.z + w * b.w)
         */
        float Dot(const Vector4& b) const {
            return x * b.x + y * b.y + z * b.z + w * b.w;
        }

        /**
         * Squared magnitude (length) of this vector
         * @return x² + y² + z² + w²
         */
        float MagnitudeSquared() const {
            return x * x + y * y + z * z + w * w;
        }

        /**
         * Magnitude (length) of this vector
         * @return sqrt(x² + y² + z² + w²)
         */
        float Magnitude() const {
            return std::sqrt(MagnitudeSquared());
        }

        /**
         * Normalized unit vector
         * @return Vector with same direction, length 1 (or zero vector if length is 0)
         */
        Vector4 Normalize() const {
            float len = Magnitude();
            return len == 0 ? Vector4(0, 0, 0, 0) : *this / len;
        }

        /**
         * Check equality with another vector
         * @param b Vector to compare
         * @return True if all components are equal
         */
        bool Equals(const Vector4& b) const {
            return *this == b;
        }
    };
}