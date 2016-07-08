#pragma once

#include "stdafx.h"

namespace Graphics
{
enum class ProjectionType
{
    Perspective,
    Orthographic
};

class SphericalCamera
{
public:
    SphericalCamera(ProjectionType type);
    SphericalCamera(ProjectionType type, float nearPlane, float farPlane, float fov, float screenWidth, float screenHeight);

    void SetCenter(const XMFLOAT3 & position);
    void SetRadius(float radius);
    void SetInclination(float inclination);
    void SetRotation(float rotation);
    void SetProjectionType(ProjectionType type);

    XMMATRIX GetViewMatrix() const;
    XMMATRIX GetProjMatrix() const;
    XMMATRIX GetViewProjMatrix() const;
    XMFLOAT4 GetEyePosition() const;

private:
    void UpdateMatrices();

    ProjectionType  _type = ProjectionType::Perspective;
    float           _zNear = 0.1f;
    float           _zFar = 100.0f;
    float           _fov = 60.0f;
    float           _aspectRatio = 16.0f / 9.0f;
    float           _screenWidth = 1024.0f;
    float           _screenHeight = 600.0f;
    XMFLOAT3        _centerPosition = {0.0f, 0.0f, 0.0f};
    float           _radius = 10.0f;
    float           _inclination = 0.0f;
    float           _rotation = 0.0f;
    XMMATRIX        _viewMatrix = XMMatrixIdentity();
    XMMATRIX        _projectionMatrix = XMMatrixIdentity();

};
}
