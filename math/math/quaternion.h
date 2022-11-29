#pragma once

#include "matrix.h"
#include "vector3.h"

namespace math
{

template<typename T>
class quaternion
{
public:
    vec3<T> vector;
    T w = (T)1.0;

    constexpr quaternion()
        : vector(T(1.0), T(0.0), T(0.0))
        , w(T(1.0))
    {
    }

    constexpr quaternion(T x, T y, T z, T w)
        : vector(x, y, z)
        , w(w)
    {
    }

    constexpr quaternion(const vec3<T> & vec)
        : vector(vec)
        , w(0)
    {
    }

    constexpr quaternion(const vec3<T> & vec, T w)
        : vector(vec)
        , w(w)
    {
    }

    quaternion(float latitude, float longitude, float angle)
    {
        float sin_a = sin(angle / 2);
        float cos_a = cos(angle / 2);

        float sin_lat = sin(latitude);
        float cos_lat = cos(latitude);

        float sin_long = sin(longitude);
        float cos_long = cos(longitude);

        vector.x = sin_a * cos_lat * sin_long;
        vector.y = sin_a * sin_lat;
        vector.z = sin_a * sin_lat * cos_long;
        w = cos_a;
    }

    constexpr quaternion operator + (const quaternion & ref) const
    {
        return quaternion(this->vector + ref.vector, this->w + ref.w);
    }

    constexpr quaternion operator - (const quaternion & ref) const
    {
        return quaternion(this->vector - ref.vector, this->w - ref.w);
    }

    quaternion operator * (const quaternion & ref) const
    {
        quaternion<T> out;
        out.vector = cross(this->vector, ref.vector) + ref.vector * this->w + this->vector * ref.w;
        out.w = this->w * ref.w - dot(this->vector, ref.vector);
        return out;
    }

    quaternion operator * (T scalar) const
    {
        quaternion<T> out = *this;
        out.vector *= scalar;
        out.w *= scalar;
        return out;
    }

    friend vec3<T> operator * (const vec3<T> & left, const quaternion<T> & right)
    {
        quaternion<T> result = right * quaternion<T>(left) * right.inverse();
        return result.to_vector();
    }

    constexpr bool operator == (const quaternion & ref) const
    {
        return this->vector == ref.vector
            && this->w == ref.w;
    }

    constexpr bool operator != (const quaternion & ref) const
    {
        return !(*this == ref);
    }

    constexpr T norm() const
    {
        return vector.x*vector.x + vector.y*vector.y + vector.z*vector.z + w*w;
    }

    T magnitude() const
    {
        return sqrt(norm());
    }

    constexpr quaternion conjugate() const
    {
        return quaternion(-vector.x, -vector.y, -vector.z, this->w);
    }

    constexpr quaternion inverse() const
    {
        return conjugate() * (T(1.0) / norm());
    }

    constexpr quaternion innerProduct(const quaternion & ref) const
    {
        return quaternion(vec3<T>(vector.x * ref.vector.x,
                                  vector.y * ref.vector.y,
                                  vector.z * ref.vector.z), w * ref.w);
    }

    quaternion normalize() const
    {
        T module = magnitude();
        return quaternion<T>(vector.x / module, vector.y / module, vector.z / module, w / module);
    }

    matrix<T, 4> to_matrix() const
    {
        float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;
        float s = 2.0f / norm();  // 4 mul 3 add 1 div
        x2 = vector.x * s;    y2 = vector.y * s;    z2 = vector.z * s;
        xx = vector.x * x2;   xy = vector.x * y2;   xz = vector.x * z2;
        yy = vector.y * y2;   yz = vector.y * z2;   zz = vector.z * z2;
        wx = w * x2;   wy = w * y2;   wz = w * z2;

        matrix<T, 4> m;

        m[0][0] = 1.0f - (yy + zz);
        m[1][0] = xy - wz;
        m[2][0] = xz + wy;

        m[0][1] = xy + wz;
        m[1][1] = 1.0f - (xx + zz);
        m[2][1] = yz - wx;

        m[0][2] = xz - wy;
        m[1][2] = yz + wx;
        m[2][2] = 1.0f - (xx + yy);

        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = 0.0f;
        m[3][3] = 1.0f;

        return m;
    }

    constexpr vec3<T> to_vector() const
    {
        return vector;
    }
};

using qaFloat = quaternion<float>;
using qaDouble = quaternion<double>;

template<typename T>
quaternion<T> quaternionFromVecAngle(const vec3<T> & axis, float angle)
{
    quaternion<T> out;
    double sin_a = sin(angle * 0.5);
    vec3<T> vec_a = axis * (T)sin_a;
    return quaternion<T>(vec_a.x, vec_a.y, vec_a.z, cos(angle * 0.5f));
}

template<typename T>
quaternion<T> quaternionFromMatrix(const matrix<T, 3> & ref)
{
    double tr, s, q[4];
    quaternion<T> out;

    tr = ref.arr[0][0] + ref.arr[1][1] + ref.arr[2][2];

    if (tr > 0.0)
    {
        s = sqrt(tr + 1.0);
        out.w = (T)(s / 2.0);
        s = 0.5 / s;
        out.vector.x = (ref.arr[1][2] - ref.arr[2][1]) * (T)s;
        out.vector.y = (ref.arr[2][0] - ref.arr[0][2]) * (T)s;
        out.vector.z = (ref.arr[0][1] - ref.arr[1][0]) * (T)s;
    }
    else
    {
        int i = 0, j, k;
        int nxt[3] = { 1, 2, 0 };

        i = 0;
        if (ref.arr[1][1] > ref.arr[0][0]) i = 1;
        if (ref.arr[2][2] > ref.arr[i][i]) i = 2;
        j = nxt[i];
        k = nxt[j];

        s = sqrt((ref.arr[i][i] - (ref.arr[j][j] + ref.arr[k][k])) + 1.0f);

        q[i] = s * 0.5;

        if (s != 0.0)
            s = 0.5 / s;

        q[3] = (ref.arr[j][k] - ref.arr[k][j]) * s;
        q[j] = (ref.arr[i][j] + ref.arr[j][i]) * s;
        q[k] = (ref.arr[i][k] + ref.arr[k][i]) * s;

        out.vector.x = (T)q[0];
        out.vector.y = (T)q[1];
        out.vector.z = (T)q[2];
        out.w = (T)q[3];
    }
    return out;
}

template<typename T>
quaternion<T> shortest_arc(const vec3<T> & from, const vec3<T> & to)
{
    quaternion<T> out;
    auto c = MathLib::cross(from, to);
    out.vector = c;
    out.w = MathLib::dot(from, to);
    out = out.normalize();
    out.w += 1.0f;
    if (out.w <= 0.00001f)
    {
        out.vector.x = 0.0;
        out.vector.y = 0.0;
        out.vector.z = 1.0;
    }
    out = out.normalize();
    return out;
}

template<typename T>
quaternion<T> quaternionSlerp(const quaternion<T> & from, const quaternion<T> & to, T t)
{
    T p1[4];
    T scale0, scale1;
    quaternion<T> out;

    double cosom = from.vector.x * to.vector.x
        + from.vector.y * to.vector.y
        + from.vector.z * to.vector.z
        + from.w * to.w;

    if (cosom < 0.0)
    {
        cosom = -cosom;
        p1[0] = -to.vector.x;
        p1[1] = -to.vector.y;
        p1[2] = -to.vector.z;
        p1[3] = -to.w;
    }
    else
    {
        p1[0] = to.vector.x;
        p1[1] = to.vector.y;
        p1[2] = to.vector.z;
        p1[3] = to.w;
    }

    if ((1.0 - cosom) > 0.001)
    {
        T omega = (T)acos(cosom);
        T sinom = (T)sin(omega);
        scale0 = (T)sin((1.0 - t) * omega) / sinom;
        scale1 = (T)sin(t * omega) / sinom;
    }
    else
    {
        scale0 = (T)1.0 - t;
        scale1 = t;
    }

    out.vector.x = scale0 * from.vector.x + scale1 * p1[0];
    out.vector.y = scale0 * from.vector.y + scale1 * p1[1];
    out.vector.z = scale0 * from.vector.z + scale1 * p1[2];
    out.w = scale0 * from.w + scale1 * p1[3];

    return out;
}

}
