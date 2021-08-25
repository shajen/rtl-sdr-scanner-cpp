cmake_minimum_required(VERSION 3.5)

project(auto-sdr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog REQUIRED)
find_package(rtlsdr REQUIRED)
find_package(Threads REQUIRED)

file(GLOB_RECURSE SRC
    "${PROJECT_SOURCE_DIR}/sources/*.h"
    "${PROJECT_SOURCE_DIR}/sources/*.cpp"
)

include_directories("${CMAKE_SOURCE_DIR}/sources")

add_executable(auto-sdr ${SRC})

TARGET_LINK_LIBRARIES(auto-sdr
    spdlog
    spdlog::spdlog
    rtlsdr
    Threads::Threads
    fftw3
    fftw3_threads
    fftw3f
    fftw3f_threads
)
