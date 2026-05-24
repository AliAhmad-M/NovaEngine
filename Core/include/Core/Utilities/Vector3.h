#pragma once
#pragma once
#include <cmath>

/**
 * 3D vector with floating point components
 */
struct Vector3 {
    float x;
    float y;
    float z;

    static const Vector3 ZERO;    // ( 0,  0,  0 )
    static const Vector3 ONE;     // ( 1,  1,  1 )
    static const Vector3 UP;      // ( 0,  1,  0 )
    static const Vector3 DOWN;    // ( 0, -1,  0 )
    static const Vector3 LEFT;    // (-1,  0,  0 )
    static const Vector3 RIGHT;   // ( 1,  0,  0 )
    static const Vector3 FORWARD; // ( 0,  0,  1 )
    static const Vector3 BACK;    // ( 0,  0, -1 )

    /**
     * Constructor
     * @param x X component (default 0)
     * @param y Y component (default 0)
     * @param z Z component (default 0)
     */
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    // Arithmetic operation overloading
    Vector3 operator+(const Vector3& b) const {
        return { x + b.x, y + b.y, z + b.z };
    }

    Vector3 operator-(const Vector3& b) const {
        return { x - b.x, y - b.y, z - b.z };
    }

    Vector3 operator*(float s) const {
        return { x * s, y * s, z * s };
    }

    Vector3 operator/(float s) const {
        return { x / s, y / s, z / s };
    }

    Vector3 operator-() const {
        return { -x, -y, -z };
    }

    Vector3& operator+=(const Vector3& b) {
        x += b.x;
        y += b.y;
        z += b.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& b) {
        x -= b.x;
        y -= b.y;
        z -= b.z;
        return *this;
    }

    Vector3& operator*=(float s) {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    Vector3& operator/=(float s) {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }

    // Comparison operator overloading
    bool operator==(const Vector3& b) const {
        return x == b.x && y == b.y && z == b.z;
    }

    bool operator!=(const Vector3& b) const {
        return !(*this == b);
    }

    bool operator>(const Vector3& b) const {
        return MagnitudeSquared() > b.MagnitudeSquared();
    }

    bool operator>=(const Vector3& b) const {
        return MagnitudeSquared() >= b.MagnitudeSquared();
    }

    bool operator<(const Vector3& b) const {
        return MagnitudeSquared() < b.MagnitudeSquared();
    }

    bool operator<=(const Vector3& b) const {
        return MagnitudeSquared() <= b.MagnitudeSquared();
    }

    // Static methods
    /**
     * Adds two vectors
     * @param a First vector
     * @param b Second vector
     * @return Vector (a + b)
     */
    static Vector3 Add(const Vector3& a, const Vector3& b) {
        return a + b;
    }

    /**
     * Subtracts two vectors
     * @param a First vector
     * @param b Second vector
     * @return Vector (a - b)
     */
    static Vector3 Subtract(const Vector3& a, const Vector3& b) {
        return a - b;
    }

    /**
     * Multiplies vector by scalar
     * @param a Vector
     * @param s Scalar value
     * @return Vector (a * s)
     */
    static Vector3 Multiply(const Vector3& a, float s) {
        return a * s;
    }

    /**
     * Divides vector by scalar
     * @param a Vector
     * @param s Scalar value (non-zero)
     * @return Vector (a / s)
     */
    static Vector3 Divide(const Vector3& a, float s) {
        return a / s;
    }

    /**
     * Negates a vector
     * @param a Vector
     * @return Vector (-a)
     */
    static Vector3 Negate(const Vector3& a) {
        return -a;
    }

    /**
     * Dot product of two vectors
     * @param a First vector
     * @param b Second vector
     * @return Dot product (a.x * b.x + a.y * b.y + a.z * b.z)
     */
    static float Dot(const Vector3& a, const Vector3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    /**
     * Cross product of two vectors
     * @param a First vector
     * @param b Second vector
     * @return Vector perpendicular to both a and b
     */
    static Vector3 Cross(const Vector3& a, const Vector3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    /**
     * Squared magnitude of a vector
     * @param a Vector
     * @return a.x² + a.y² + a.z²
     */
    static float MagnitudeSquared(const Vector3& a) {
        return a.x * a.x + a.y * a.y + a.z * a.z;
    }

    /**
     * Magnitude (length) of a vector
     * @param a Vector
     * @return sqrt(a.x² + a.y² + a.z²)
     */
    static float Magnitude(const Vector3& a) {
        return std::sqrt(MagnitudeSquared(a));
    }

    /**
     * Normalized unit vector
     * @param a Vector
     * @return Vector with same direction, length 1 (or zero vector if length is 0)
     */
    static Vector3 Normalize(const Vector3& a) {
        float len = Magnitude(a);
        return len == 0 ? Vector3(0, 0, 0) : a / len;
    }

    /**
     * Linear interpolation between two vectors
     * @param a Start vector
     * @param b End vector
     * @param t Interpolation factor (0 to 1)
     * @return Interpolated vector
     */
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
        return a + (b - a) * t;
    }

    /**
     * Distance between two points
     * @param a First point
     * @param b Second point
     * @return Distance between a and b
     */
    static float Distance(const Vector3& a, const Vector3& b) {
        return Magnitude(b - a);
    }

    /**
     * Squared distance between two points
     * @param a First point
     * @param b Second point
     * @return Squared distance between a and b
     */
    static float DistanceSquared(const Vector3& a, const Vector3& b) {
        return MagnitudeSquared(b - a);
    }

    /**
     * Reflects a vector off a surface with given normal
     * @param v Incident vector
     * @param n Surface normal (should be normalized)
     * @return Reflected vector
     */
    static Vector3 Reflect(const Vector3& v, const Vector3& n) {
        return v - n * (2.0f * Dot(v, n));
    }

    /**
     * Projects vector a onto vector b
     * @param a Vector to project
     * @param b Vector to project onto
     * @return Projected vector
     */
    static Vector3 Project(const Vector3& a, const Vector3& b) {
        float bMagSq = MagnitudeSquared(b);
        if (bMagSq == 0) return Vector3(0, 0, 0);
        return b * (Dot(a, b) / bMagSq);
    }

    /**
     * Checks if two vectors are equal
     * @param a First vector
     * @param b Second vector
     * @return True if a.x == b.x and a.y == b.y and a.z == b.z
     */
    static bool Equals(const Vector3& a, const Vector3& b) {
        return a == b;
    }

    // Instance methods
    /**
     * Adds another vector to this vector
     * @param b Vector to add
     * @return New vector as (*this + b)
     */
    Vector3 Add(const Vector3& b) const {
        return *this + b;
    }

    /**
     * Subtracts another vector from this vector
     * @param b Vector to subtract
     * @return New vector as (*this - b)
     */
    Vector3 Subtract(const Vector3& b) const {
        return *this - b;
    }

    /**
     * Multiplies this vector by a scalar
     * @param s Scalar value
     * @return New vector as (*this * s)
     */
    Vector3 Multiply(float s) const {
        return *this * s;
    }

    /**
     * Divides this vector by a scalar
     * @param s Scalar value (non-zero)
     * @return New vector as (*this / s)
     */
    Vector3 Divide(float s) const {
        return *this / s;
    }

    /**
     * Negates this vector
     * @return New vector as (-*this)
     */
    Vector3 Negate() const {
        return -*this;
    }

    /**
     * Dot product with another vector
     * @param b Other vector
     * @return Dot product (x * b.x + y * b.y + z * b.z)
     */
    float Dot(const Vector3& b) const {
        return x * b.x + y * b.y + z * b.z;
    }

    /**
     * Cross product with another vector
     * @param b Other vector
     * @return Vector perpendicular to both this and b
     */
    Vector3 Cross(const Vector3& b) const {
        return {
            y * b.z - z * b.y,
            z * b.x - x * b.z,
            x * b.y - y * b.x
        };
    }

    /**
     * Squared magnitude (length) of this vector
     * @return x² + y² + z²
     */
    float MagnitudeSquared() const {
        return x * x + y * y + z * z;
    }

    /**
     * Magnitude (length) of this vector
     * @return sqrt(x² + y² + z²)
     */
    float Magnitude() const {
        return std::sqrt(MagnitudeSquared());
    }

    /**
     * Normalized unit vector
     * @return Vector with same direction, length 1 (or zero vector if length is 0)
     */
    Vector3 Normalize() const {
        float len = Magnitude();
        return len == 0 ? Vector3(0, 0, 0) : *this / len;
    }

    /**
     * Check equality with another vector
     * @param b Vector to compare
     * @return True if x == b.x and y == b.y and z == b.z
     */
    bool Equals(const Vector3& b) const {
        return *this == b;
    }
};