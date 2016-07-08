set(SRC
    CommandList.cpp
    CommandList.h
    ComputePipelineState.cpp
    ComputePipelineState.h
    DXSampleHelper.h
    FeaturesCollector.h
    Math.h
    GraphicsPipelineState.cpp
    GraphicsPipelineState.h
    MeshManager.cpp
    MeshManager.h
    RenderTargetManager.cpp
    RenderTargetManager.h
    RootSignature.cpp
    RootSignature.h
    SceneObject.cpp
    SceneObject.h
    Shaders.h
    SphericalCamera.cpp
    SphericalCamera.h
    stdafx.h
    Types.h
    )

add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /Yc")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Yc")

add_library(utils STATIC ${SRC})

target_link_libraries(utils dxgi.lib d3d12.lib d3dcompiler.lib)
