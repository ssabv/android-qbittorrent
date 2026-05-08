cmake_minimum_required(VERSION 3.22.1)

project(qbcore VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/../../../third_party)
set(LIBTORRENT_DIR ${THIRD_PARTY_DIR}/libtorrent-install)
set(BOOST_DIR ${THIRD_PARTY_DIR}/boost-install)
set(OPENSSL_DIR ${THIRD_PARTY_DIR}/openssl-install)

add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
add_compile_options(-fPIC -fvisibility=hidden)
add_compile_options(-fno-exceptions -fno-rtti)

find_library(log-lib log)
find_library(android-lib android)
find_library(jni-lib jni)
find_library(openssl-lib crypto)
find_library(ssl-lib ssl)
find_library(z-lib z)
find_library(pthread-lib pthread)

include_directories(
    ${CMAKE_SOURCE_DIR}/qbittorrent
    ${LIBTORRENT_DIR}/include
    ${BOOST_DIR}/include
    ${OPENSSL_DIR}/include
)

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

if(EXISTS ${LIBTORRENT_DIR}/lib/libtorrent.a)
    message(STATUS "Using prebuilt libtorrent from ${LIBTORRENT_DIR}")
    target_link_libraries(qbcore
        ${LIBTORRENT_DIR}/lib/libtorrent.a
    )
else()
    message(WARNING "libtorrent not found at ${LIBTORRENT_DIR}, using system")
    find_library(lt-lib libtorrent)
    if(lt-lib)
        target_link_libraries(qbcore ${lt-lib})
    endif()
endif()

target_link_libraries(qbcore
    ${log-lib}
    ${android-lib}
    ${jni-lib}
    ${openssl-lib}
    ${ssl-lib}
    ${z-lib}
    ${pthread-lib}
    atomic
)

add_custom_command(TARGET qbcore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Built qBittorrent Core"
    COMMENT "qBittorrent Android Native Library Built"
)