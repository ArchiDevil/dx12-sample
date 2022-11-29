set (SHADERS
    ../assets/shaders/AOPass.hlsl
    ../assets/shaders/Blur.hlsl
    ../assets/shaders/ColorSpaceConverters.hlsl
    ../assets/shaders/DepthPass.hlsl
    ../assets/shaders/IntensityPass.hlsl
    ../assets/shaders/LDRPass.hlsl
    ../assets/shaders/LightPass.hlsl
    ../assets/shaders/MRTPass.hlsl
)

set(SRC
    DX12Sample.cpp
    DX12Sample.h
    DXSample.cpp
    DXSample.h
    main.cpp
    SceneManager.cpp
    SceneManager.h
    stdafx.h
    ${SHADERS}
)

set_property(SOURCE ${SHADERS} PROPERTY HEADER_FILE_ONLY TRUE)

add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

add_executable(dx12_sample ${SRC})

target_link_libraries(dx12_sample
    3rdparty
    utils
    winpixruntime
    dxgi.lib
    d3d12.lib
    d3dcompiler.lib)

if (MSVC14)
    if (${EX_PLATFORM} EQUAL 64)
        target_link_libraries(dx12_sample ${CMAKE_CURRENT_LIST_DIR}/../3rdparty/assimp/lib/x64/vs2015/${CMAKE_CFG_INTDIR}/assimp-vc140-mt.lib)
    elseif(${EX_PLATFORM} EQUAL 32)
        target_link_libraries(dx12_sample ${CMAKE_CURRENT_LIST_DIR}/../3rdparty/assimp/lib/x86/vs2015/${CMAKE_CFG_INTDIR}/assimp-vc140-mt.lib)
    endif()
else()
    message(FATAL_ERROR "This target is only support Visual Studio 2015, update assimp libraries first (right now we use 3.3.1 version, please use the same or update libraries for all versions of Visual Studio")
endif()

set_target_properties(dx12_sample PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
set_target_properties(dx12_sample PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
set_target_properties(dx12_sample PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
set_target_properties(dx12_sample PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")

add_dependencies(dx12_sample copy_assets)

source_group("Shaders" FILES ${SHADERS})
