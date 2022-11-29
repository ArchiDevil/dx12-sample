#pragma once

#include <cmath>

namespace math
{

template<typename T>
struct vec2
{
    constexpr vec2()
        : x(0)
        , y(0)
    {
    }

    constexpr vec2(T _x, T _y)
        : x(_x)
        , y(_y)
    {
    }

    union
    {
        T el[2];
        struct
        {
            T x, y;
        };
    };

    constexpr T length() const
    {
        return (T)sqrt((double)x*x + (double)y*y);
    }

    constexpr const T * ptr() const
    {
        return el;
    }

    inline T * ptr()
    {
        return el;
    }

    inline vec2 & operator += (const vec2 & ref)
    {
        this->x += ref.x;
        this->y += ref.y;
        return *this;
    }

    inline vec2 & operator -= (const vec2 & ref)
    {
        this->x -= ref.x;
        this->y -= ref.y;
        return *this;
    }

    template<typename D>
    inline vec2 & operator *= (D num)
    {
        this->x *= (T)num;
        this->y *= (T)num;
        return *this;
    }

    template<typename T2>
    inline vec2 & operator = (vec2<T2> & vec)
    {
        this->x = (T)vec.x;
        this->y = (T)vec.y;
        return *this;
    }

    constexpr vec2 operator + (const vec2 & ref) const
    {
        return{ this->x + ref.x, this->y + ref.y };
    }

    constexpr vec2 operator - (const vec2 & ref) const
    {
        return{ this->x - ref.x, this->y - ref.y };
    }

    constexpr vec2 operator - () const
    {
        return{ -x, -y };
    }

    template<typename T2>
    constexpr operator vec2<T2>() const
    {
        return vec2<T2>((T2)x, (T2)y);
    }

    template<typename D>
    constexpr vec2 operator * (D num) const
    {
        return{ x * (T)num, y * (T)num };
    }

    constexpr bool operator == (const vec2 & ref) const
    {
        return ((this->x == ref.x) && (this->y == ref.y));
    }

    constexpr bool operator != (const vec2 & ref) const
    {
        return !((this->x == ref.x) && (this->y == ref.y));
    }
};

using vec2d = vec2<double>;
using vec2f = vec2<float>;
using vec2i = vec2<int>;

//dot product between two vectors
template <typename T>
constexpr T dot(const vec2<T> & vec1, const vec2<T> & vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y;
}

//normalizes vector
template<typename T>
constexpr vec2<T> normalize(const vec2<T> & vec)
{
    return vec * ((T)1.0 / vec.length());
}

//angle between two vectors
template<typename T>
T angle(const vec2<T> &vec1, const vec2<T> &vec2)
{
    return acos((T)dot(vec1, vec2) / (T)vec1.length() / (T)vec2.length());
}

//distance between two points
template<typename T>
T distance(const vec2<T> & arg1, const vec2<T> & arg2)
{
    return (T)sqrt((T)(arg1.x - arg2.x)*(arg1.x - arg2.x) +
        (T)(arg1.y - arg2.y)*(arg1.y - arg2.y));
}

}
