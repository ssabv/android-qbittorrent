cmake_minimum_required(VERSION 3.22.1)

project(qbcore VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_SOURCE_DIR})

find_library(log-lib log)

add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
add_compile_options(-fPIC -fvisibility=hidden)

# OpenSSL from environment variable OPENSSL_DIR
set(OPENSSL_ROOT_DIR "$ENV{OPENSSL_DIR}")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
set(OPENSSL_LIB_DIR "${OPENSSL_ROOT_DIR}/lib")

# Boost (optional)
set(BOOST_DIR "$ENV{BOOST_DIR}")
set(BOOST_LIB_DIR "$ENV{BOOST_LIB_DIR}")

if(EXISTS "${BOOST_DIR}")
    set(BOOST_INCLUDE_DIR "${BOOST_DIR}")
    if(EXISTS "${BOOST_DIR}/include/boost")
        set(BOOST_INCLUDE_DIR "${BOOST_DIR}/include")
    elseif(EXISTS "${BOOST_DIR}/libs/arm64-v8a")
        set(BOOST_INCLUDE_DIR "${BOOST_DIR}/libs/arm64-v8a")
    endif()
    include_directories(${BOOST_INCLUDE_DIR})
endif()

if(EXISTS "${OPENSSL_INCLUDE_DIR}")
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

add_library(
    qbcore
    SHARED
    native-lib.cpp
    qbittorrent/qbcore.cpp
)

target_compile_options(qbcore PRIVATE
    -ffunction-sections
    -fdata-sections
)

target_link_options(qbcore PRIVATE
    -Wl,--gc-sections
)

# Link system libs only. C++ STL handled by ANDROID_STL cmake variable.
target_link_libraries(qbcore
    ${log-lib}
)

# Link OpenSSL static libs
if(EXISTS "${OPENSSL_LIB_DIR}/libssl.a")
    target_link_libraries(qbcore
        "${OPENSSL_LIB_DIR}/libssl.a"
        "${OPENSSL_LIB_DIR}/libcrypto.a"
    )
else()
    message(WARNING "OpenSSL static libs not found at ${OPENSSL_LIB_DIR}")
endif()

# Link Boost (optional)
if(BOOST_LIB_DIR AND EXISTS "${BOOST_LIB_DIR}")
    if(EXISTS "${BOOST_LIB_DIR}/libboost_system.a")
        target_link_libraries(qbcore
            "${BOOST_LIB_DIR}/libboost_system.a"
            "${BOOST_LIB_DIR}/libboost_filesystem.a"
            "${BOOST_LIB_DIR}/libboost_date_time.a"
            "${BOOST_LIB_DIR}/libboost_thread.a"
            "${BOOST_LIB_DIR}/libboost_regex.a"
        )
    endif()
endif()

add_custom_command(TARGET qbcore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Built qBittorrent Core"
    COMMENT "qBittorrent Android Native Library Built"
)
