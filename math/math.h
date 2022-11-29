#pragma once

#include "math/vector2.h"
#include "math/vector3.h"
#include "math/vector4.h"
#include "math/quaternion.h"
#include "math/matrix.h"

#include "math/matrixFuncs.h"

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#define M_PIF 3.14159265f
#endif

namespace math
{

template<typename T>
T clamp(T value, T min, T max)
{
    if (value > max)
        return max;
    if (value < min)
        return min;
    return value;
}

//returns angle between vector and X-axis
template<typename T>
T angleX(const vec2<T> & vec)
{
    vec2<T> tempVec = normalize(vec);
    if (asin(tempVec.y) >= 0)
        return acos(tempVec.x);
    else
        return -acos(tempVec.x);
}

template<typename T>
constexpr T LinearInterpolation(T a, T b, T k)
{
    return a + (b - a) * k;
}

template<typename T>
inline T CosineInterpolation(T a, T b, T k)
{
    T ft = k * (T)M_PI;
    T f = (1 - std::cos(ft)) * 0.5f;
    return a*(1 - f) + b*f;
}

template<typename T>
inline vec3<T> GetPointOnSphere(vec3<T> center, T radius, T xAngle, T yAngle)
{
    T x = radius * std::sin(yAngle * (T)0.0175) * std::cos(xAngle * (T)0.0175) + center.x;
    T y = radius * std::sin(yAngle * (T)0.0175) * std::sin(xAngle * (T)0.0175) + center.y;
    T z = radius * std::cos(yAngle * (T)0.0175) + center.z;
    return vec3<T>(x, y, z);
}

template<typename T>
constexpr T degrad(T degress)
{
    return degress * (T)M_PI / (T)180.0;
}

template<typename T>
constexpr T raddeg(T radians)
{
    return radians * (T)180.0 / (T)M_PI;
}

template<class T,
    class = typename std::enable_if<std::is_floating_point<T>::value>::type>
bool areEqual(T a, T b)
{
    return std::abs(a - b) < std::numeric_limits<T>::epsilon();
}

template<typename T>
vec3<T> getUnprojectedVector(const vec3<T> & v, const matrix<T, 4> & projMat, const matrix<T, 4> & viewMat, const vec2<unsigned int> & screenSize)
{
    matrix<T, 4> rm = viewMat * projMat;
    matrix<T, 4> resultMatrix = matrixInverse(rm);
    vec3f result;

    int TopLeftX = 0;
    int TopLeftY = 0;
    unsigned int Width = screenSize.x;
    unsigned int Height = screenSize.y;
    float MinDepth = 0.0f;
    float MaxDepth = 1.0f;

    result.x = (v.x - TopLeftX) / Width * 2.0f - 1.0f;
    result.y = -(2.0f * (v.y - TopLeftY) / Height - 1.0f);
    result.z = (v.z - MinDepth) / (MaxDepth - MinDepth);
    result = vec3Transform(result, resultMatrix);
    return result;
}

}
