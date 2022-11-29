#pragma once

#include <cmath>

namespace math
{

template<typename T>
struct vec3
{
    constexpr vec3()
        : x(0)
        , y(0)
        , z(0)
    {
    }

    constexpr vec3(T _x, T _y, T _z)
        : x(_x)
        , y(_y)
        , z(_z)
    {
    }

    union
    {
        T el[3];
        struct
        {
            T x, y, z;
        };
    };

    constexpr T length() const
    {
        return (T)sqrt((double)x*x + (double)y*y + (double)z*z);
    }

    inline T * ptr()
    {
        return el;
    }

    constexpr const T * ptr() const
    {
        return el;
    }

    inline vec3 & operator += (const vec3 & ref)
    {
        this->x += ref.x;
        this->y += ref.y;
        this->z += ref.z;
        return *this;
    }

    inline vec3 & operator -= (const vec3 & ref)
    {
        this->x -= ref.x;
        this->y -= ref.y;
        this->z -= ref.z;
        return *this;
    }

    template<typename D>
    inline vec3 & operator *= (D num)
    {
        this->x *= (T)num;
        this->y *= (T)num;
        this->z *= (T)num;
        return *this;
    }

    template<typename T2>
    inline vec3 & operator = (const vec3<T2> & vec)
    {
        this->x = (T)vec.x;
        this->y = (T)vec.y;
        this->z = (T)vec.z;
        return *this;
    }

    template<typename T2>
    constexpr operator vec3<T2>() const
    {
        return vec3<T2>((T2)x, (T2)y, (T2)z);
    }

    constexpr vec3 operator + (const vec3 & ref) const
    {
        return{ this->x + ref.x, this->y + ref.y, this->z + ref.z };
    }

    constexpr vec3 operator - (const vec3 & ref) const
    {
        return{ this->x - ref.x, this->y - ref.y, this->z - ref.z };
    }

    constexpr vec3 operator - () const
    {
        return{ -x, -y, -z };
    }

    template<typename D>
    constexpr vec3 operator * (D num) const
    {
        return{ x * (T)num, y * (T)num, z * (T)num };
    }

    constexpr bool operator == (const vec3 & ref) const
    {
        return ((this->x == ref.x) && (this->y == ref.y) && (this->z == ref.z));
    }

    constexpr bool operator != (const vec3 & ref) const
    {
        return !((this->x == ref.x) && (this->y == ref.y) && (this->z == ref.z));
    }
};

using vec3f = vec3<float>;
using vec3d = vec3<double>;
using vec3i = vec3<int>;

//dot product between two vectors
template <typename T>
constexpr T dot(const vec3<T> & vec1, const vec3<T> & vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}

//cross product between two vectors
template<typename T>
constexpr vec3<T> cross(const vec3<T> & vec1, const vec3<T> & vec2)
{
    return vec3<T>(vec1.y * vec2.z - vec1.z * vec2.y, vec1.z * vec2.x - vec1.x * vec2.z, vec1.x * vec2.y - vec1.y * vec2.x);
}

//normalizes vector
template<typename T>
constexpr vec3<T> normalize(const vec3<T> & vec)
{
    return vec * ((T)1.0 / vec.length());
}

//angle between two vectors
template<typename T>
T angle(const vec3<T> &vec1, const vec3<T> &vec2)
{
    return acos((T)dot(vec1, vec2) / (T)vec1.length() / (T)vec2.length());
}

//normalized vector projected onto X
template<typename T>
vec3<T> projX(const vec3<T> &vec)
{
    vec3<T> out = vec;
    out.x = 0.0f;
    out = normalize(out);
    return out;
}

//normalized vector projected onto Y
template<typename T>
vec3<T> projY(const vec3<T> &vec)
{
    vec3<T> out = vec;
    out.y = 0.0f;
    out = normalize(out);
    return out;
}

//normalized vector projected onto Z
template<typename T>
vec3<T> projZ(const vec3<T> &vec)
{
    vec3<T> out = vec;
    out.z = 0.0f;
    out = normalize(out);
    return out;
}

//distance between two points
template<typename T>
T distance(const vec3<T> & vec1, const vec3<T> & vec2)
{
    return (T)sqrt((T)(vec1.x - vec2.x)*(vec1.x - vec2.x) +
                   (T)(vec1.y - vec2.y)*(vec1.y - vec2.y) +
                   (T)(vec1.z - vec2.z)*(vec1.z - vec2.z));
}

}
