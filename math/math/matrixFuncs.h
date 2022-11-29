#pragma once

#include "matrix.h"

#include <cmath>

namespace math
{

//D3DXColorAdjustContrast
//D3DXColorAdjustSaturation
//D3DXFloat16To32Array
//D3DXFloat32To16Array
//D3DXFresnelTerm

//D3DXPlaneFromPointNormal
//D3DXPlaneFromPoints
//D3DXPlaneIntersectLine
//D3DXPlaneNormalize
//D3DXPlaneTransform
//D3DXPlaneTransformArray

//D3DXQuaternionBaryCentric
//D3DXQuaternionExp
//D3DXQuaternionInverse
//D3DXQuaternionLn
//D3DXQuaternionMultiply
//D3DXQuaternionNormalize
//D3DXQuaternionRotationAxis
//D3DXQuaternionRotationMatrix
//D3DXQuaternionRotationYawPitchRoll
//D3DXQuaternionSlerp
//D3DXQuaternionSquad
//D3DXQuaternionSquadSetup
//D3DXQuaternionToAxisAngle

//D3DXSHEvalConeLight
//D3DXSHEvalDirection
//D3DXSHEvalDirectionalLight
//D3DXSHEvalHemisphereLight
//D3DXSHMultiply2
//D3DXSHMultiply3
//D3DXSHMultiply4
//D3DXSHMultiply5
//D3DXSHMultiply6
//D3DX10SHProjectCubeMap
//D3DXSHRotate
//D3DXSHRotateZ
//D3DXSHScale

//D3DXVec2BaryCentric
//D3DXVec2CatmullRom
//D3DXVec2Hermite
//D3DXVec2Transform
//D3DXVec2TransformArray
//D3DXVec2TransformCoord
//D3DXVec2TransformCoordArray
//D3DXVec2TransformNormal
//D3DXVec2TransformNormalArray

//D3DXVec3BaryCentric
//D3DXVec3CatmullRom
//D3DXVec3Hermite
//D3DXVec3Project
//D3DXVec3ProjectArray
//D3DXVec3Transform
//D3DXVec3TransformArray
//D3DXVec3TransformCoord
//D3DXVec3TransformCoordArray
//D3DXVec3TransformNormal
//D3DXVec3TransformNormalArray

//D3DXVec4BaryCentric
//D3DXVec4CatmullRom
//D3DXVec4Cross
//D3DXVec4Hermite
//D3DXVec4Transform
//D3DXVec4TransformArray

//D3DXMatrixAffineTransformation 
//D3DXMatrixAffineTransformation2D 
//D3DXMatrixDecompose

template <typename T, size_t E>
matrix<T, E> matrixIdentity()
{
    matrix<T, E> out;
    for (size_t i = 0; i < E; i++)
    {
        out[i][i] = (T)1;
    }
    return out;
}

template <typename T>
matrix<T, 3> matrixScaling(const vec2<T> & v)
{
    return matrixScaling(v.x, v.y);
}

template <typename T>
matrix<T, 3> matrixScaling(T x, T y)
{
    matrix<T, 3> out;
    out[0][0] = x;
    out[1][1] = y;
    out[2][2] = (T)1;
    return out;
}

template <typename T>
matrix<T, 4> matrixScaling(const vec3<T> & v)
{
    return matrixScaling(v.x, v.y, v.z);
}

template <typename T>
matrix<T, 4> matrixScaling(T x, T y, T z)
{
    matrix<T, 4> out;
    out[0][0] = x;
    out[1][1] = y;
    out[2][2] = z;
    out[3][3] = (T)1;
    return out;
}

template<typename T>
matrix<T, 4> matrixRotationX(T value)
{
    matrix<T, 4> matX;
    matX[0][0] = (T)1;
    matX[1][1] = cos(value);
    matX[1][2] = sin(value);
    matX[2][1] = -sin(value);
    matX[2][2] = cos(value);
    matX[3][3] = (T)1;
    return matX;
}

template<typename T>
matrix<T, 4> matrixRotationY(T value)
{
    matrix<T, 4> matY;
    matY[0][0] = cos(value);
    matY[0][2] = -sin(value);
    matY[1][1] = (T)1;
    matY[2][0] = sin(value);
    matY[2][2] = cos(value);
    matY[3][3] = (T)1;
    return matY;
}

template<typename T>
matrix<T, 4> matrixRotationZ(T value)
{
    matrix<T, 4> matZ;
    matZ[0][0] = cos(value);
    matZ[0][1] = sin(value);
    matZ[1][0] = -sin(value);
    matZ[1][1] = cos(value);
    matZ[2][2] = (T)1;
    matZ[3][3] = (T)1;
    return matZ;
}

template<typename T>
matrix<T, 4> matrixRotationYPR(T yaw, T pitch, T roll)
{
    matrix<T, 4> out;
    out[0][0] = (cos(roll) * cos(yaw)) + (sin(roll) * sin(pitch) * sin(yaw));
    out[0][1] = (sin(roll) * cos(pitch));
    out[0][2] = (cos(roll) * -sin(yaw)) + (sin(roll) * sin(pitch) * cos(yaw));
    out[1][0] = (-sin(roll) * cos(yaw)) + (cos(roll) * sin(pitch) * sin(yaw));
    out[1][1] = (cos(roll) * cos(pitch));
    out[1][2] = (sin(roll) * sin(yaw)) + (cos(roll) * sin(pitch) * cos(yaw));
    out[2][0] = (cos(pitch) * sin(yaw));
    out[2][1] = -sin(pitch);
    out[2][2] = (cos(pitch) * cos(yaw));
    out[3][3] = (T)1;
    return out;
}

template <typename T>
matrix<T, 3> matrixTranslation(const vec2<T> & v)
{
    return matrixTranslation(v.x, v.y);
}

template <typename T>
matrix<T, 3> matrixTranslation(T x, T y)
{
    matrix<T, 3> out;
    out[0][0] = (T)1;
    out[1][1] = (T)1;
    out[2][2] = (T)1;

    out[2][0] = x;
    out[2][1] = y;
    return out;
}

template <typename T>
matrix<T, 4> matrixTranslation(const vec3<T> & v)
{
    return matrixTranslation(v.x, v.y, v.z);
}

template <typename T>
matrix<T, 4> matrixTranslation(T x, T y, T z)
{
    matrix<T, 4> out;
    out[0][0] = (T)1;
    out[1][1] = (T)1;
    out[2][2] = (T)1;
    out[3][3] = (T)1;

    out[3][0] = x;
    out[3][1] = y;
    out[3][2] = z;
    return out;
}

template<typename T>
vec2<T> vec2Transform(const vec2<T> & v, const matrix<T, 4> & m)
{
    float norm = m[0][3] * v.x + m[1][3] * v.y + m[3][3];
    vec2<T> out;

    if (norm)
    {
        out.x = (m[0][0] * v.x + m[1][0] * v.y + m[3][0]) / norm;
        out.y = (m[0][1] * v.x + m[1][1] * v.y + m[3][1]) / norm;
    }
    else
    {
        out.x = 0.0f;
        out.y = 0.0f;
    }
    return out;
}

template<typename T>
vec3<T> vec3Transform(const vec3<T> & v, const matrix<T, 4> & m)
{
    float norm = m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3];
    vec3<T> out;

    if (norm)
    {
        out.x = (m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0]) / norm;
        out.y = (m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1]) / norm;
        out.z = (m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2]) / norm;
    }
    else
    {
        out.x = 0.0f;
        out.y = 0.0f;
        out.z = 0.0f;
    }
    return out;
}

template<typename T>
vec4<T> vec4Transform(const vec4<T> & v, const matrix<T, 4> & m)
{
    return m * v;
}

template<typename T, size_t E>
T matrixDeterminant(const matrix<T, E> & ref)
{
    T result = (T)0.0;
    for (size_t i = 0; i < E; ++i)
        result += ref[0][i] * ((i % 2) ? -1 : 1) * matrixDeterminant(ref.matrixMinor(0, i));
    return result;
}

template<typename T>
T matrixDeterminant(const matrix<T, 2> & ref)
{
    return ref[0][0] * ref[1][1] - ref[0][1] * ref[1][0];
}

template<typename T, size_t E>
matrix<T, E> matrixInverse(const matrix<T, E> & ref)
{
    // http://www.mathsisfun.com/algebra/matrix-inverse-minors-cofactors-adjugate.html
    matrix<T, E> out;
    // calculate matrix of minors
    for (size_t i = 0; i < E; ++i)
        for (size_t j = 0; j < E; ++j)
            out[i][j] = matrixDeterminant(ref.matrixMinor(i, j));

    // calculate matrix of cofactors
    for (size_t i = 0; i < E; ++i)
        for (size_t j = 0; j < E; ++j)
            out[i][j] = (((i + j) % 2) ? -1 : 1) * out[i][j];

    // adjugate
    out = out.transpose();

    // multiple by 1/det
    out = out / matrixDeterminant(ref);
    return out;
}

template<typename T>
matrix<T, 4> matrixLookAtLH(const vec3<T> & eye, const vec3<T> & at, const vec3<T> & up)
{
    vec3<T> zaxis = normalize(at - eye);
    vec3<T> xaxis = normalize(cross(up, zaxis));
    vec3<T> yaxis = cross(zaxis, xaxis);

    matrix<T, 4> out;
    out[0][0] = xaxis.x;            out[0][1] = yaxis.x;            out[0][2] = zaxis.x;            out[0][3] = 0;
    out[1][0] = xaxis.y;            out[1][1] = yaxis.y;            out[1][2] = zaxis.y;            out[1][3] = 0;
    out[2][0] = xaxis.z;            out[2][1] = yaxis.z;            out[2][2] = zaxis.z;            out[2][3] = 0;
    out[3][0] = -dot(xaxis, eye);   out[3][1] = -dot(yaxis, eye);   out[3][2] = -dot(zaxis, eye);   out[3][3] = 1;
    return out;
}

template<typename T>
matrix<T, 4> matrixLookAtRH(const vec3<T> & eye, const vec3<T> & at, const vec3<T> & up)
{
    vec3<T> zaxis = normalize(eye - at);
    vec3<T> xaxis = normalize(cross(up, zaxis));
    vec3<T> yaxis = cross(zaxis, xaxis);

    matrix<T, 4> out;
    out[0][0] = xaxis.x;            out[0][1] = yaxis.x;            out[0][2] = zaxis.x;            out[0][3] = 0;
    out[1][0] = xaxis.y;            out[1][1] = yaxis.y;            out[1][2] = zaxis.y;            out[1][3] = 0;
    out[2][0] = xaxis.z;            out[2][1] = yaxis.z;            out[2][2] = zaxis.z;            out[2][3] = 0;
    out[3][0] = -dot(xaxis, eye);   out[3][1] = -dot(yaxis, eye);   out[3][2] = -dot(zaxis, eye);   out[3][3] = 1;
    return out;
}

// TODO: recheck, it just does not work in most cases, something is wrong here!
template<typename T>
matrix<T, 4> matrixOrthoLH(float w, float h, float zf, float zn)
{
    matrix<T, 4> out;
    out[0][0] = 2 / w;  out[0][1] = 0;      out[0][2] = 0;                  out[0][3] = 0;
    out[1][0] = 0;      out[1][1] = 2 / h;  out[1][2] = 0;                  out[1][3] = 0;
    out[2][0] = 0;      out[2][1] = 0;      out[2][2] = 1 / (zf - zn);      out[2][3] = 0;
    out[3][0] = 0;      out[3][1] = 0;      out[3][2] = -zn / (zn - zf);    out[3][3] = 1;
    return out;
}

// TODO: recheck, it just does not work in most cases, something is wrong here!
template<typename T>
matrix<T, 4> matrixOrthoRH(float w, float h, float zf, float zn)
{
    matrix<T, 4> out;
    out[0][0] = 2 / w;  out[0][1] = 0;      out[0][2] = 0;              out[0][3] = 0;
    out[1][0] = 0;      out[1][1] = 2 / h;  out[1][2] = 0;              out[1][3] = 0;
    out[2][0] = 0;      out[2][1] = 0;      out[2][2] = 1 / (zn - zf);  out[2][3] = 0;
    out[3][0] = 0;      out[3][1] = 0;      out[3][2] = zn / (zn - zf); out[3][3] = 1;
    return out;
}

//************************************
// Method:    matrixOrthoOffCenterLH
// FullName:  MathLib::matrixOrthoOffCenterLH
// Access:    public 
// Returns:   Builds a customized, left-handed orthographic projection matrix.
// Qualifier:
// Parameter: float l - Minimum x-value of view volume.
// Parameter: float r -Maximum x - value of view volume.
// Parameter: float b - Minimum y - value of view volume.
// Parameter: float t - Maximum y - value of view volume.
// Parameter: float zn - Minimum z-value of the view volume.
// Parameter: float zf - Maximum z-value of the view volume.
//************************************
template<typename T>
matrix<T, 4> matrixOrthoOffCenterLH(float l, float r, float b, float t, float zn, float zf)
{
    matrix<T, 4> out;
    out[0][0] = 2 / (r - l);        out[0][1] = 0;                  out[0][2] = 0;              out[0][3] = 0;
    out[1][0] = 0;                  out[1][1] = 2 / (t - b);        out[1][2] = 0;              out[1][3] = 0;
    out[2][0] = 0;                  out[2][1] = 0;                  out[2][2] = 1 / (zf - zn);  out[2][3] = 0;
    out[3][0] = (l + r) / (l - r);  out[3][1] = (t + b) / (b - t);  out[3][2] = zn / (zn - zf); out[3][3] = 1;
    return out;
}

//************************************
// Method:    matrixOrthoOffCenterRH
// FullName:  MathLib::matrixOrthoOffCenterRH
// Access:    public 
// Returns:   Builds a customized, right-handed orthographic projection matrix.
// Qualifier:
// Parameter: float l - Minimum x-value of view volume.
// Parameter: float r - Maximum x-value of view volume.
// Parameter: float b - Minimum y-value of view volume.
// Parameter: float t - Maximum y-value of view volume.
// Parameter: float zn - Minimum z-value of the view volume.
// Parameter: float zf - Maximum z-value of the view volume.
//************************************
template<typename T>
matrix<T, 4> matrixOrthoOffCenterRH(float l, float r, float b, float t, float zn, float zf)
{
    matrix<T, 4> out;
    out[0][0] = 2 / (r - l);        out[0][1] = 0;                  out[0][2] = 0;              out[0][3] = 0;
    out[1][0] = 0;                  out[1][1] = 2 / (t - b);        out[1][2] = 0;              out[1][3] = 0;
    out[2][0] = 0;                  out[2][1] = 0;                  out[2][2] = 1 / (zn - zf);  out[2][3] = 0;
    out[3][0] = (l + r) / (l - r);  out[3][1] = (t + b) / (b - t);  out[3][2] = zn / (zn - zf); out[3][3] = 1;
    return out;
}

template<typename T>
matrix<T, 4> matrixPerspectiveFovLH(float fovy, float aspectRatio, float zn, float zf)
{
    float yScale = 1.0f / std::tan(fovy / 2.0f);
    float xScale = yScale / aspectRatio;

    matrix<T, 4> out;
    out[0][0] = xScale; out[0][1] = 0;      out[0][2] = 0;                      out[0][3] = 0;
    out[1][0] = 0;      out[1][1] = yScale; out[1][2] = 0;                      out[1][3] = 0;
    out[2][0] = 0;      out[2][1] = 0;      out[2][2] = zf / (zf - zn);         out[2][3] = 1;
    out[3][0] = 0;      out[3][1] = 0;      out[3][2] = -zn * zf / (zf - zn);   out[3][3] = 0;
    return out;
}

template<typename T>
matrix<T, 4> matrixPerspectiveFovRH(float fovy, float aspectRatio, float zn, float zf)
{
    float yScale = 1.0f / std::tan(fovy / 2.0f);
    float xScale = yScale / aspectRatio;

    matrix<T, 4> out;
    out[0][0] = xScale; out[0][1] = 0;      out[0][2] = 0;                      out[0][3] = 0;
    out[1][0] = 0;      out[1][1] = yScale; out[1][2] = 0;                      out[1][3] = 0;
    out[2][0] = 0;      out[2][1] = 0;      out[2][2] = zf / (zn - zf);         out[2][3] = -1;
    out[3][0] = 0;      out[3][1] = 0;      out[3][2] = zn * zf / (zn - zf);    out[3][3] = 0;
    return out;
}

//D3DXMatrixReflect 
//D3DXMatrixRotationAxis 
//D3DXMatrixShadow 
}
