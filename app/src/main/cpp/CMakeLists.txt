cmake_minimum_required(VERSION 3.22.1)

project(qbcore VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Precompiled libraries directory
set(PREBUILT_LIBS_DIR ${CMAKE_SOURCE_DIR}/../../../jniLibs/${ANDROID_ABI})

# Find Android system libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(jni-lib jni)
find_library(z-lib z)
find_library(pthread-lib pthread)

# Import precompiled libtorrent
add_library(libtorrent SHARED IMPORTED)
set_target_properties(libtorrent PROPERTIES
    IMPORTED_LOCATION ${PREBUILT_LIBS_DIR}/libtorrent.so
)

# Import precompiled OpenSSL
add_library(ssl SHARED IMPORTED)
set_target_properties(ssl PROPERTIES
    IMPORTED_LOCATION ${PREBUILT_LIBS_DIR}/libssl.so
)

add_library(crypto SHARED IMPORTED)
set_target_properties(crypto PROPERTIES
    IMPORTED_LOCATION ${PREBUILT_LIBS_DIR}/libcrypto.so
)

# Import Boost libraries
add_library(boost_system SHARED IMPORTED)
set_target_properties(boost_system PROPERTIES
    IMPORTED_LOCATION ${PREBUILT_LIBS_DIR}/libboost_system.so
)

add_library(boost_filesystem SHARED IMPORTED)
set_target_properties(boost_filesystem PROPERTIES
    IMPORTED_LOCATION ${PREBUILT_LIBS_DIR}/libboost_filesystem.so
)

add_library(boost_date_time SHARED IMPORTED)
set_target_properties(boost_date_time PROPERTIES
    IMPORTED_LOCATION ${PREBUILT_LIBS_DIR}/libboost_date_time.so
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/qbittorrent
)

# Compile options
add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
add_compile_options(-fPIC -fvisibility=hidden)
add_compile_options(-fno-exceptions -fno-rtti)

# Build native library
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
    -Wl,--as-needed
)

# Link libraries
target_link_libraries(qbcore
    libtorrent
    ssl
    crypto
    boost_system
    boost_filesystem
    boost_date_time
    ${log-lib}
    ${android-lib}
    ${jni-lib}
    ${z-lib}
    ${pthread-lib}
    atomic
)

add_custom_command(TARGET qbcore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Built qBittorrent Core"
    COMMENT "qBittorrent Android Native Library Built"
)