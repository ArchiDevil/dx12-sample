#include "stdafx.h"

#include "SphericalCamera.h"

#include "Math.h"

#define M_PI 3.14159265f

namespace Graphics
{
SphericalCamera::SphericalCamera(ProjectionType type)
    : _type(type)
{
    UpdateMatrices();
}

SphericalCamera::SphericalCamera(ProjectionType type, float nearPlane, float farPlane, float fov, float screenWidth, float screenHeight)
    : _type(type)
    , _zNear(nearPlane)
    , _zFar(farPlane)
    , _fov(fov)
    , _aspectRatio(screenWidth / screenHeight)
    , _screenWidth(screenWidth)
    , _screenHeight(screenHeight)
{
    UpdateMatrices();
}

void SphericalCamera::SetCenter(const XMFLOAT3 & position)
{
    _centerPosition = position;
    UpdateMatrices();
}

void SphericalCamera::SetRadius(float radius)
{
    _radius = radius;
    UpdateMatrices();
}

void SphericalCamera::SetInclination(float inclination)
{
    _inclination = inclination;
    UpdateMatrices();
}

void SphericalCamera::SetRotation(float rotation)
{
    _rotation = rotation;
    UpdateMatrices();
}

void SphericalCamera::SetProjectionType(ProjectionType type)
{
    _type = type;
    UpdateMatrices();
}

DirectX::XMMATRIX SphericalCamera::GetViewMatrix() const
{
    return _viewMatrix;
}

DirectX::XMMATRIX SphericalCamera::GetProjMatrix() const
{
    return _projectionMatrix;
}

DirectX::XMMATRIX SphericalCamera::GetViewProjMatrix()const
{
    return _viewMatrix * _projectionMatrix;
}

DirectX::XMFLOAT4 SphericalCamera::GetEyePosition() const
{
    return Math::GetPointOnSphere(_centerPosition, _radius, _rotation, _inclination);
}

void SphericalCamera::UpdateMatrices()
{
    XMFLOAT4 camPos = Math::GetPointOnSphere(_centerPosition, _radius, _rotation, _inclination);
    XMVECTOR m = {camPos.x, camPos.y, camPos.z, camPos.w};

    if (_type == ProjectionType::Perspective)
        _projectionMatrix = XMMatrixPerspectiveFovLH(_fov, _aspectRatio, _zNear, _zFar);
    else if (_type == ProjectionType::Orthographic)
        _projectionMatrix = XMMatrixOrthographicLH(_screenWidth, _screenHeight, _zNear, _zFar);

    _viewMatrix = XMMatrixLookAtLH(m, {_centerPosition.x, _centerPosition.y, _centerPosition.z, 1.0}, {0.0, 1.0, 0.0, 1.0});
}
}
