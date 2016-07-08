cmake_minimum_required(VERSION 2.8)
project(dx12_sample)

include_directories(.)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets/ ${CMAKE_BINARY_DIR}/dx12_sample/${CMAKE_CFG_INTDIR}/assets/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets/ ${CMAKE_BINARY_DIR}/dx12_sample/assets/)

add_subdirectory(3rdparty)
add_subdirectory(dx12_sample)
add_subdirectory(utils)
