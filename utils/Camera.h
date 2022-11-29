#pragma once

#include <math/math.h>

#include <memory>

namespace Graphics
{

static const float MAX_ANGLE = 89.5f;

enum class ProjectionType
{
    Perspective,
    Orthographic
};

class Camera final
{
public:
    Camera(float screenWidth, float screenHeight, float zNear, float zFar, float FOV, ProjectionType viewType);

    void SetPosition(const math::vec3f & pos);
    void RotateByLocalQuaternion(const math::qaFloat & quat);

    void Update();
    void MoveUpDown(float units);
    void MoveLeftRight(float units);
    void MoveForwardBackward(float units);
    void LookAt(const math::vec3f & point);
    void SetSphericalCoords(const math::vec3f & lookPoint, float rotation, float inclination, float radius);

    const math::vec3f & GetPosition() const;
    const math::vec3f & GetLookVector() const;
    const math::vec3f & GetRightVector() const;
    const math::vec3f & GetUpVector() const;
    const math::mat4f & GetProjectionMatrix() const;
    const math::mat4f & GetViewMatrix() const;
    const math::mat4f GetViewProjMatrix() const;

    void SetZNear(float val);
    float GetZNear() const;

    void SetZFar(float val);
    float GetZFar() const;

    void SetFOV(float val);
    float GetFOV() const;

    void SetScreenWidth(float val);
    void SetScreenHeight(float val);

private:
    void RebuildProjMatrix();

    math::mat4f matView;
    math::mat4f matProj;

    float zNear = 0.1f;
    float zFar = 100.0f;
    float fov = 60.0f;
    float screenWidth = 800.0f;
    float screenHeight = 600.0f;
    ProjectionType viewType = ProjectionType::Perspective;

    float viewAngle = 0.0f;
    math::vec3f position = {};
    math::vec3f angles = {};
    math::vec3f upVector = { 0.0f, 1.0f, 0.0f };
    math::vec3f lookVector = { 0.0f, 0.0f, 1.0f };
    math::vec3f rightVector = math::cross(lookVector, upVector);
};

}
