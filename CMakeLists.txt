cmake_minimum_required(VERSION 3.9)
project(wfc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(DEFAULT_BUILD_TYPE "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
endif()

set(SRC src/wave.cpp src/propagator.cpp src/wfc.cpp)

add_library(${PROJECT_NAME}_static STATIC ${SRC})
add_library(${PROJECT_NAME} SHARED ${SRC})

target_include_directories(${PROJECT_NAME}_static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>)

