cmake_minimum_required(VERSION 3.16)
project(dx12_sample)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(.)
add_compile_definitions(_UNICODE UNICODE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets/ ${CMAKE_BINARY_DIR}/dx12_sample/${CMAKE_CFG_INTDIR}/assets/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets/ ${CMAKE_BINARY_DIR}/dx12_sample/assets/)

add_subdirectory(3rdparty)
add_subdirectory(dx12_sample)
add_subdirectory(utils)
