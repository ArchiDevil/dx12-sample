add_library(winpixruntime SHARED IMPORTED GLOBAL)

set_target_properties(winpixruntime PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/bin/x64/WinPixEventRuntime.dll"
    IMPORTED_IMPLIB "${CMAKE_CURRENT_LIST_DIR}/bin/x64/WinPixEventRuntime.lib")

target_include_directories(winpixruntime INTERFACE "${CMAKE_CURRENT_LIST_DIR}/Include")
target_compile_definitions(winpixruntime INTERFACE USE_PIX)

add_custom_target(copy_pix
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/bin/x64/WinPixEventRuntime.dll" "${CMAKE_BINARY_DIR}/dx12_sample/${CMAKE_CFG_INTDIR}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/bin/x64/WinPixEventRuntime.dll" "${CMAKE_BINARY_DIR}/dx12_sample/")

add_dependencies(winpixruntime copy_pix)
