cmake_minimum_required(VERSION 3.22.1)

project(qbcore VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
add_compile_options(-fPIC -fvisibility=hidden)

add_library(
    qbcore
    SHARED
    native-lib.cpp
    qbittorrent/qbcore.cpp
    qbittorrent/cJSON.c
)

target_compile_options(qbcore PRIVATE
    -ffunction-sections
    -fdata-sections
)

target_link_options(qbcore PRIVATE
    -Wl,--gc-sections
)

add_custom_command(TARGET qbcore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Built qBittorrent Core"
    COMMENT "qBittorrent Android Native Library Built"
)