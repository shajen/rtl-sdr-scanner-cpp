cmake_minimum_required(VERSION 3.5)

project(auto-sdr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog REQUIRED)
find_package(rtlsdr REQUIRED)
find_package(Threads REQUIRED)

find_library(LIQUID_LIB liquid)
if(NOT LIQUID_LIB)
  message(FATAL_ERROR "liquid library not found")
endif()

find_library(SOX_LIB sox)
if(NOT SOX_LIB)
  message(FATAL_ERROR "sox library not found")
endif()

find_library(SOXR_LIB soxr)
if(NOT SOXR_LIB)
  message(FATAL_ERROR "soxr library not found")
endif()

file(GLOB_RECURSE SRC
    "${PROJECT_SOURCE_DIR}/sources/*.h"
    "${PROJECT_SOURCE_DIR}/sources/*.cpp"
)

include_directories("${CMAKE_SOURCE_DIR}/sources")

add_executable(auto-sdr ${SRC})

TARGET_LINK_LIBRARIES(auto-sdr
    spdlog::spdlog
    rtlsdr
    Threads::Threads
    liquid
    sox
    soxr
    stdc++fs
)

install(TARGETS auto-sdr DESTINATION)
