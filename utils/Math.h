#pragma once

#include "stdafx.h"

namespace Math
{

inline DirectX::XMFLOAT4 GetPointOnSphere(const XMFLOAT3 & center, float radius, float xAngle, float yAngle)
{
    float x = radius * std::sin(yAngle * 0.0175f) * std::cos(xAngle * 0.0175f) + center.x;
    float y = radius * std::sin(yAngle * 0.0175f) * std::sin(xAngle * 0.0175f) + center.y;
    float z = radius * std::cos(yAngle * 0.0175f) + center.z;
    return {x, z, y, 1.0f};
}

}
