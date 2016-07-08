float3 xyY2RGB(float3 xyY)
{
    // convert to XYZ space
    float3 XYZ = 0.0;
    XYZ.r = xyY.b * xyY.r / xyY.g;
    XYZ.g = xyY.b;
    XYZ.b = xyY.b / xyY.g * (1.0 - xyY.r - xyY.g);

    // convert to RGB space
    float3x3 RGBMatrix = float3x3( 2.3706743, -0.9000405, -0.4706338,
                                  -0.5138850,  1.4253036,  0.0885814,
                                   0.0052982, -0.0146949,  1.0093968);
    float3 RGB = mul(XYZ, RGBMatrix);
    return RGB;
}

float3 RGB2xyY(float3 RGB)
{
    float3 XYZ = 0.0;
    // convert to XYZ space
    float3x3 XYZMatrix = float3x3(0.4887180, 0.3106803, 0.2006017,
                                  0.1762044, 0.8129847, 0.0108109,
                                  0.0000000, 0.0102048, 0.9897952);
    XYZ = mul(RGB, XYZMatrix);
    // convert to xyY
    float3 xyY = 0.0;
    xyY.r = XYZ.x / (XYZ.x + XYZ.y + XYZ.z); // x
    xyY.g = XYZ.y / (XYZ.x + XYZ.y + XYZ.z); // y
    xyY.b = XYZ.y;                           // Y
    return xyY;
}
