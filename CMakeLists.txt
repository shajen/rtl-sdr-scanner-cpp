cmake_minimum_required(VERSION 3.5)

project(auto_sdr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Wpedantic -Wno-missing-braces")

find_package(spdlog REQUIRED)
find_package(Gnuradio COMPONENTS
    blocks
    fft
    filter
    analog
    digital
    audio
    pdu
    video-sdl
    network
    soapy
)

file(GLOB_RECURSE SOURCES
    "${PROJECT_SOURCE_DIR}/sources/*.h"
    "${PROJECT_SOURCE_DIR}/sources/*.cpp"
)
file(GLOB_RECURSE TEST_SOURCES
    "${PROJECT_SOURCE_DIR}/tests/*.cpp"
)
list(REMOVE_ITEM SOURCES "${PROJECT_SOURCE_DIR}/sources/main.cpp")
list(REMOVE_ITEM TEST_SOURCES "${PROJECT_SOURCE_DIR}/tests/test_main.cpp")

include(GNUInstallDirs)
include_directories("${CMAKE_SOURCE_DIR}/sources")

add_library(auto_sdr_libs STATIC ${SOURCES})

add_executable(auto_sdr "sources/main.cpp")
target_link_libraries(auto_sdr
    auto_sdr_libs
    spdlog::spdlog
    gnuradio::gnuradio-blocks
    gnuradio::gnuradio-filter
    gnuradio::gnuradio-soapy
    gnuradio::gnuradio-fft
)

add_executable(auto_sdr_test ${TEST_SOURCES} "tests/test_main.cpp")
target_link_libraries(auto_sdr_test auto_sdr_libs gtest)

install(TARGETS auto_sdr DESTINATION)
install(TARGETS auto_sdr_test DESTINATION)
