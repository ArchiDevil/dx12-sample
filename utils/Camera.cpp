#include "stdafx.h"
#include "Camera.h"

using namespace math;
using namespace Graphics;

Camera::Camera(float screenWidth, float screenHeight, float zNear, float zFar, float FOV, ProjectionType viewType)
    : zNear(zNear)
    , zFar(zFar)
    , fov(FOV)
    , screenWidth(screenWidth)
    , screenHeight(screenHeight)
    , viewType(viewType)
{
    matView = matrixLookAtLH<float>(position, lookVector + position, upVector);
    RebuildProjMatrix();
}

void Camera::SetPosition(const vec3f & pos)
{
    position = pos;
    Update();
}

void Camera::MoveUpDown(float units)
{
    position += upVector * units;
    Update();
}

void Camera::MoveLeftRight(float units)
{
    position += rightVector * units;
    Update();
}

void Camera::MoveForwardBackward(float units)
{
    position += lookVector * units;
    Update();
}

void Camera::Update()
{
    vec3f eye = lookVector + position;
    matView = matrixLookAtLH<float>(position, eye, upVector);
}

const math::vec3f & Camera::GetLookVector() const
{
    return lookVector;
}

const math::vec3f & Camera::GetRightVector() const
{
    return rightVector;
}

void Camera::LookAt(const vec3f & point)
{
    lookVector = normalize(point);
    Update();
}

const mat4f & Camera::GetProjectionMatrix() const
{
    return matProj;
}

const mat4f & Camera::GetViewMatrix() const
{
    return matView;
}

const math::vec3f & Camera::GetUpVector() const
{
    return upVector;
}

float Camera::GetZNear() const
{
    return zNear;
}

float Camera::GetZFar() const
{
    return zFar;
}

float Camera::GetFOV() const
{
    return fov;
}

void Camera::SetZFar(float val)
{
    zFar = val;
    RebuildProjMatrix();
}

void Camera::SetZNear(float val)
{
    zNear = val;
    RebuildProjMatrix();
}

void Camera::SetFOV(float val)
{
    fov = val;
    RebuildProjMatrix();
}

void Camera::SetScreenWidth(float val)
{
    screenWidth = val;
    RebuildProjMatrix();
}

void Camera::SetScreenHeight(float val)
{
    screenHeight = val;
    RebuildProjMatrix();
}

void Camera::RebuildProjMatrix()
{
    switch (viewType)
    {
    case ProjectionType::Perspective:
        matProj = matrixPerspectiveFovLH<float>(fov,       // vertical FoV
                                                screenWidth / screenHeight, // screen rate
                                                zNear,
                                                zFar);
        break;
    case ProjectionType::Orthographic:
        matProj = matrixOrthoLH<float>(screenWidth, screenHeight, zFar, zNear);
        break;
    default:
        break;
    }
}

void Camera::RotateByLocalQuaternion(const qaFloat & quat)
{
    // transform all vectors
    vec3f look = {lookVector.x, lookVector.y, lookVector.z};
    look = look * quat;
    lookVector = look;

    vec3f up = {upVector.x, upVector.y, upVector.z};
    up = up * quat;
    upVector = up;

    rightVector = math::cross(lookVector, upVector);

    Update();
}

void Camera::SetSphericalCoords(const vec3f & center, float rotation, float inclination, float radius)
{
    position = GetPointOnSphere(center, radius, rotation, inclination);
    position = { position.x, position.z, position.y };
    lookVector = center - position;
    lookVector = normalize(lookVector);
    // we assume that UP vector is always direct to up
    upVector = {0.0f, 1.0f, 0.0f};
    rightVector = cross(lookVector, upVector);
    rightVector = normalize(rightVector);
    upVector = cross(rightVector, lookVector);
    upVector = normalize(upVector);

    Update();
}

const math::vec3f & Camera::GetPosition() const
{
    return position;
}

const math::mat4f Graphics::Camera::GetViewProjMatrix() const
{
    return matView * matProj;
}
