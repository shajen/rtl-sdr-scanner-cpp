cmake_minimum_required(VERSION 3.5)

project(auto-sdr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Wpedantic -Wno-missing-braces]")

find_package(spdlog REQUIRED)
find_package(rtlsdr REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

find_library(LIQUID_LIB liquid)
if(NOT LIQUID_LIB)
  message(FATAL_ERROR "liquid library not found")
endif()

find_library(MOSQUITTO_LIB mosquitto)
if(NOT MOSQUITTO_LIB)
  message(FATAL_ERROR "mosquitto library not found")
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
    mosquitto
    stdc++fs
    nlohmann_json::nlohmann_json
)

install(TARGETS auto-sdr DESTINATION)
