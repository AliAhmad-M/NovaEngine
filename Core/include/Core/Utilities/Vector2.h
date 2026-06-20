#pragma once
#include <cmath>

namespace Nova 
{
    /**
 * @brief 2D vector with floating point components
 */
    struct Vector2 {
        float x;
        float y;

        static const Vector2 ZERO;  // ( 0,  0 )
        static const Vector2 ONE;   // ( 1,  1 )
        static const Vector2 UP;    // ( 0, -1 )
        static const Vector2 LEFT;  // (-1,  0 )
        static const Vector2 DOWN;  // ( 0,  1 )
        static const Vector2 RIGHT; // ( 1,  0 )

        /**
         * Constructor
         * @param x X component (default 0)
         * @param y Y component (default 0)
         */
        Vector2(float x = 0, float y = 0) : x(x), y(y) {}


        Vector2 operator+(const Vector2& b) const
        {
            return { x + b.x, y + b.y };
        }

        Vector2 operator-(const Vector2& b) const
        {
            return { x - b.x, y - b.y };
        }

        Vector2 operator*(float s) const
        {
            return { x * s, y * s };
        }

        Vector2 operator/(float s) const
        {
            return { x / s, y / s };
        }

        Vector2 operator-() const
        {
            return { -x, -y };
        }

        Vector2& operator+=(const Vector2& b)
        {
            x += b.x;
            y += b.y;
            return *this;
        }

        Vector2& operator-=(const Vector2& b)
        {
            x -= b.x;
            y -= b.y;
            return *this;
        }

        Vector2& operator*=(float s)
        {
            x *= s;
            y *= s;
            return *this;
        }

        Vector2& operator/=(float s)
        {
            x /= s;
            y /= s;
            return *this;
        }

        bool operator==(const Vector2& b) const
        {
            return x == b.x && y == b.y;
        }

        bool operator!=(const Vector2& b) const
        {
            return !(*this == b);
        }

        bool operator>(const Vector2& b) const
        {
            return MagnitudeSquared() > b.MagnitudeSquared();
        }

        bool operator>=(const Vector2& b) const
        {
            return MagnitudeSquared() >= b.MagnitudeSquared();
        }

        bool operator<(const Vector2& b) const
        {
            return MagnitudeSquared() < b.MagnitudeSquared();
        }

        bool operator<=(const Vector2& b) const
        {
            return MagnitudeSquared() <= b.MagnitudeSquared();
        }

        // Static methods
        /**
         * Adds two vectors
         * @param a First vector
         * @param b Second vector
         * @return Vector (a + b)
         */
        static Vector2 Add(const Vector2& a, const Vector2& b)
        {
            return a + b;
        }

        /**
         * Subtracts two vectors
         * @param a First vector
         * @param b Second vector
         * @return Vector (a - b)
         */
        static Vector2 Subtract(const Vector2& a, const Vector2& b)
        {
            return a - b;
        }

        /**
         * Multiplies vector by scalar
         * @param a Vector
         * @param s Scalar value
         * @return Vector (a * s)
         */
        static Vector2 Multiply(const Vector2& a, float s)
        {
            return a * s;
        }

        /**
         * Divides vector by scalar
         * @param a Vector
         * @param s Scalar value (non-zero)
         * @return Vector (a / s)
         */
        static Vector2 Divide(const Vector2& a, float s)
        {
            return a / s;
        }

        /**
         * Negates a vector
         * @param a Vector
         * @return Vector (-a)
         */
        static Vector2 Negate(const Vector2& a)
        {
            return -a;
        }

        /**
         * Dot product of two vectors
         * @param a First vector
         * @param b Second vector
         * @return Dot product (a.x * b.x + a.y * b.y)
         */
        static float Dot(const Vector2& a, const Vector2& b)
        {
            return a.x * b.x + a.y * b.y;
        }

        /**
         * 2D cross product of two vectors (z-component of the 3D cross product)
         * @param a First vector
         * @param b Second vector
         * @return 2D cross product scalar value (a.x * b.y - a.y * b.x)
         */
        static float Cross(const Vector2& a, const Vector2& b)
        {
            return a.x * b.y - a.y * b.x;
        }

        /**
         * Squared magnitude of a vector
         * @param a Vector
         * @return a.x² + a.y²
         */
        static float MagnitudeSquared(const Vector2& a)
        {
            return a.x * a.x + a.y * a.y;
        }

        /**
         * Magnitude (length) of a vector
         * @param a Vector
         * @return sqrt(a.x² + a.y²)
         */
        static float Magnitude(const Vector2& a)
        {
            return std::sqrt(MagnitudeSquared(a));
        }

        /**
         * Normalized unit vector
         * @param a Vector
         * @return Vector with same direction, length 1 (or zero vector if length is 0)
         */
        static Vector2 Normalize(const Vector2& a)
        {
            float len = Magnitude(a);
            return len == 0 ? Vector2(0, 0) : a / len;
        }

        /**
         * Checks if two vectors are equal
         * @param a First vector
         * @param b Second vector
         * @return True if a.x == b.x and a.y == b.y
         */
        static bool Equals(const Vector2& a, const Vector2& b)
        {
            return a == b;
        }

        // Instance methods
        /**
         * Adds another vector to this vector
         * @param b Vector to add
         * @return New vector as (*this + b)
         */
        Vector2 Add(const Vector2& b) const
        {
            return *this + b;
        }

        /**
         * Subtracts another vector from this vector
         * @param b Vector to subtract
         * @return New vector as (*this - b)
         */
        Vector2 Subtract(const Vector2& b) const
        {
            return *this - b;
        }

        /**
         * Multiplies this vector by a scalar
         * @param s Scalar value
         * @return New vector as (*this * s)
         */
        Vector2 Multiply(float s) const
        {
            return *this * s;
        }

        /**
         * Divides this vector by a scalar
         * @param s Scalar value (non-zero)
         * @return New vector as (*this / s)
         */
        Vector2 Divide(float s) const
        {
            return *this / s;
        }

        /**
         * Negates this vector
         * @return New vector as (-*this)
         */
        Vector2 Negate() const
        {
            return -*this;
        }

        /**
         * Dot product with another vector
         * @param b Other vector
         * @return Dot product (x * b.x + y * b.y)
         */
        float Dot(const Vector2& b) const
        {
            return x * b.x + y * b.y;
        }

        /**
         * 2D cross product with another vector (z-component of  3D cross product)
         * @param b Other vectorthe
         * @return 2D cross product scalar value (x * b.y - y * b.x)
         */
        float Cross(const Vector2& b) const
        {
            return x * b.y - y * b.x;
        }

        /**
         * Squared magnitude (length) of this vector
         * @return x² + y²
         */
        float MagnitudeSquared() const
        {
            return x * x + y * y;
        }

        /**
         * Magnitude (length) of this vector
         * @return sqrt(x² + y²)
         */
        float Magnitude() const
        {
            return std::sqrt(MagnitudeSquared());
        }

        /**
         * Normalized unit vector
         * @return Vector with same direction, length 1 (or zero vector if length is 0)
         */
        Vector2 Normalize() const
        {
            float len = Magnitude();
            return len == 0 ? Vector2(0, 0) : *this / len;
        }

        /**
         * Check equality with another vector
         * @param b Vector to compare
         * @return True if x == b.x and y == b.y
         */
        bool Equals(const Vector2& b) const
        {
            return *this == b;
        }
    };
}