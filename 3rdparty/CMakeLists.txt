set(SRC
    d3dx12.h
    DDSTextureLoader.cpp
    DDSTextureLoader.h
    DDS.h
    PlatformHelpers.h
)

add_library(3rdparty STATIC ${SRC})

add_subdirectory(winpixeventruntime.1.0.200127001)

if (${EX_PLATFORM} EQUAL 64)
    add_custom_command(TARGET 3rdparty POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/assimp/lib/x64/vs2015/${CMAKE_CFG_INTDIR}/assimp-vc140-mt.dll
                                                      ${CMAKE_BINARY_DIR}/dx12_sample/${CMAKE_CFG_INTDIR}/assimp-vc140-mt.dll

        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/assimp/lib/x64/vs2015/${CMAKE_CFG_INTDIR}/assimp-vc140-mt.dll
                                                      ${CMAKE_BINARY_DIR}/dx12_sample/assimp-vc140-mt.dll

        COMMENT "Updating plugin interface..." VERBATIM)
elseif(${EX_PLATFORM} EQUAL 32)
    add_custom_command(TARGET 3rdparty POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/assimp/lib/x86/vs2015/${CMAKE_CFG_INTDIR}/assimp-vc140-mt.dll
                                                      ${CMAKE_BINARY_DIR}/dx12_sample/${CMAKE_CFG_INTDIR}/assimp-vc140-mt.dll

        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/assimp/lib/x86/vs2015/${CMAKE_CFG_INTDIR}/assimp-vc140-mt.dll
                                                      ${CMAKE_BINARY_DIR}/dx12_sample/assimp-vc140-mt.dll

        COMMENT "Updating plugin interface..." VERBATIM)

endif()
