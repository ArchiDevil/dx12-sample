set(SRC
    d3dx12.h
    DDSTextureLoader.cpp
    DDSTextureLoader.h
    DDS.h
    PlatformHelpers.h
)

add_library(3rdparty STATIC ${SRC})

add_subdirectory(winpixeventruntime.1.0.200127001)
