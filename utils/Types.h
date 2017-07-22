#pragma once

enum optTypes
{
    disable_bundles,
    disable_concurrency,
    disable_textures,
    disable_root_constants,
    disable_shadow_pass,
    enable_tessellation,
    legacy_swapchain,
};

enum class ShaderType
{
    Vertex,
    Pixel,
    Geometry,
    Hull,
    Domain,
    Compute
};

struct geometryVertex
{
    float position[3];
    float normal[3];
    float binormal[3];
    float tangent[3];
    float uv[2];
};

struct screenQuadVertex
{
    float position[2];
    float uv[2];
};

struct sceneParamsConstantBuffer
{
    float lightPosition[4];

    float ambientColor[3];
    float depthMapSize;

    float fogColor[3];
    float sceneSize;

    float shadowMatrix[4][4];

    float inverseViewProj[4][4];

    float camPosition[4];
};

struct perModelParamsConstantBuffer
{
    float worldMatrix[4][4];
};

struct perFrameParamsConstantBuffer
{
    float viewProjectionMatrix[4][4];
    float cameraPosition[4];
};

struct CommandLineOptions
{
    bool threads = true;
    bool bundles = true;
    bool root_constants = true;
    bool shadow_pass = true;
    bool textures = true;
    bool tessellation = false;
    bool legacy_swapchain = false;
};
