set(SRC
    DX12Sample.cpp
    DX12Sample.h
    DXSample.cpp
    DXSample.h
    main.cpp
    SceneManager.cpp
    SceneManager.h
    stdafx.h
    )

add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

add_executable(dx12_sample ${SRC})

target_link_libraries(dx12_sample 3rdparty utils dxgi.lib d3d12.lib d3dcompiler.lib)

set_target_properties(dx12_sample PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
set_target_properties(dx12_sample PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
set_target_properties(dx12_sample PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
set_target_properties(dx12_sample PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")

add_dependencies(dx12_sample copy_assets)
