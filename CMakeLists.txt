cmake_minimum_required(VERSION 3.5)

project(auto_sdr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the local Find*.cmake scripts
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Wpedantic -Wno-missing-braces")

find_package(spdlog REQUIRED)
find_package(SoapySDR REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)
find_package(Boost REQUIRED)
set(CMAKE_CXX_STANDARD 17) # due to SoapySDR c++11

find_library(FFTW_LIB fftw3)
if(NOT FFTW_LIB)
  message(FATAL_ERROR "fftw3 library not found")
endif()

find_library(LIQUID_LIB liquid)
if(NOT LIQUID_LIB)
  message(FATAL_ERROR "liquid library not found")
endif()

find_library(MOSQUITTO_LIB mosquitto)
if(NOT MOSQUITTO_LIB)
  message(FATAL_ERROR "mosquitto library not found")
endif()

file(GLOB_RECURSE SOURCES
    "${PROJECT_SOURCE_DIR}/sources/*.h"
    "${PROJECT_SOURCE_DIR}/sources/*.cpp"
)
file(GLOB_RECURSE TEST_SOURCES
    "${PROJECT_SOURCE_DIR}/tests/*.cpp"
)
list(REMOVE_ITEM SOURCES "${PROJECT_SOURCE_DIR}/sources/main.cpp")
list(REMOVE_ITEM TEST_SOURCES "${PROJECT_SOURCE_DIR}/tests/test_main.cpp")

include_directories("${CMAKE_SOURCE_DIR}/sources")

add_library(auto_sdr_libs STATIC ${SOURCES})
TARGET_LINK_LIBRARIES(auto_sdr_libs
    spdlog::spdlog
    Threads::Threads
    fftw3_threads
    fftw3
    fftw3f_threads
    fftw3f
    liquid
    mosquitto
    stdc++fs
    nlohmann_json::nlohmann_json
    SoapySDR
)

add_executable(auto_sdr "sources/main.cpp")
target_link_libraries(auto_sdr auto_sdr_libs)

add_executable(auto_sdr_test ${TEST_SOURCES} "tests/test_main.cpp")
target_link_libraries(auto_sdr_test auto_sdr_libs gtest)

install(TARGETS auto_sdr DESTINATION)
install(TARGETS auto_sdr_test DESTINATION)
