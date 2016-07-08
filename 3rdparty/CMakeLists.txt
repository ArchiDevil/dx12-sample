set(SRC
    d3dx12.h
    DDSTextureLoader.cpp
    DDSTextureLoader.h
    DDS.h
    PlatformHelpers.h
    )

add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

add_library(3rdparty STATIC ${SRC})
